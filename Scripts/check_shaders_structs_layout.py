from dataclasses import dataclass
from pathlib import Path
import re
import sys


BASE_DIR = Path(__file__).parent.parent
INCLUDE_DIR = BASE_DIR / "Include"
SHADER_DIR = BASE_DIR / "Assets" / "Shaders"

# Struct name -> tuple of (cpp header, list of shader files)
STRUCTS = {
    "Particle": (
        INCLUDE_DIR / "ParticleSystem.h",
        [
            SHADER_DIR / "Compute" / "Particle.comp",
            SHADER_DIR / "Particle.vert",
        ],
    ),
    "DirectionalLight": (
        INCLUDE_DIR / "Components" / "DirectionalLight.h",
        [
            SHADER_DIR / "PBR.frag",
        ],
    ),
}

CPP_FIELDS_SIZE_AND_ALIGNMENT = {
    "float": (4, 4),
    "int32_t": (4, 4),
    "uint32_t": (4, 4),
    "Color": (16, 16),
    "glm::vec2": (8, 8),
    "glm::vec3": (12, 16),
    "glm::vec4": (16, 16),
    "glm::mat2": (32, 16),
    "glm::mat3": (48, 16),
    "glm::mat4": (64, 16),
}

GLSL_FIELDS_SIZE_AND_ALIGNMENT = {
    "float": (4, 4),
    "int": (4, 4),
    "uint": (4, 4),
    "vec2": (8, 8),
    "vec3": (12, 16),
    "vec4": (16, 16),
    "mat2": (32, 16),
    "mat3": (48, 16),
    "mat4": (64, 16),
}

CPP_GLSL_FIELDS_MAPPING = {
    "float": "float",
    "int32_t": "int",
    "uint32_t": "uint",
    "Color": "vec4",
    "glm::vec2": "vec2",
    "glm::vec3": "vec3",
    "glm::vec4": "vec4",
    "glm::mat2": "mat2",
    "glm::mat3": "mat3",
    "glm::mat4": "mat4",
}

def find_cpp_struct_fields(cpp_source: str, struct_name: str) -> tuple[int | None, list[tuple[str, str, int | None]]]:
    # This regex matches the struct definition and captures the fields
    struct_pattern = re.compile(
        rf"struct(?:\s+alignas\(\s*(\d+)\s*\))?\s+{struct_name}\s*{{(.*?)}};",
        re.DOTALL
    )
    match = struct_pattern.search(cpp_source)
    if not match:
        raise ValueError(f"Struct {struct_name} not found in C++ source.")
    
    struct_alignas = int(match.group(1)) if match.group(1) else None
    fields_str = match.group(2)
    field_pattern = re.compile(r"(?:alignas\(\s*(\d+)\s*\)\s+)?([A-Za-z_][\w:<>,]*)\s+(\w+)\s*;")
    fields = field_pattern.findall(fields_str)
    
    fields = [
        (
            field_type.strip(),
            field_name.strip(),
            int(explicit_alignas) if explicit_alignas else None,
        )
        for explicit_alignas, field_type, field_name in fields
    ]
    return struct_alignas, fields

def find_glsl_struct_fields(glsl_source: str, struct_name: str) -> list[tuple[str, str]]:
    # This regex matches the struct definition and captures the fields
    struct_pattern = re.compile(
        rf"struct\s+{struct_name}\s*{{(.*?)}};",
        re.DOTALL
    )
    match = struct_pattern.search(glsl_source)
    if not match:
        raise ValueError(f"Struct {struct_name} not found in GLSL source.")
    
    fields_str = match.group(1)
    field_pattern = re.compile(r"(\w[\w\s]*)\s+(\w+);")
    fields = field_pattern.findall(fields_str)
    
    return [(field_type.strip(), field_name.strip()) for field_type, field_name in fields]

def snake_case_to_camel_case(snake_str: str) -> str:
    components = snake_str.split('_')
    return components[0] + "".join(component[:1].upper() + component[1:] for component in components[1:])

errors_count = 0

for struct_name, (cpp, shaders) in STRUCTS.items():
    cpp_fields = {} # field_name -> (glsl_field_type, offset)
    
    cpp_struct_alignas, cpp_fields_list = find_cpp_struct_fields(cpp.read_text(), struct_name)
    offset = 0
    cpp_struct_alignment = 1
    for field_type, field_name, explicit_alignas in cpp_fields_list:
        if field_type not in CPP_FIELDS_SIZE_AND_ALIGNMENT:
            print(f"Unsupported C++ field type '{field_type}' in struct '{struct_name}'. Skipping.")
            errors_count += 1
            continue
        
        glsl_field_type = CPP_GLSL_FIELDS_MAPPING[field_type]
        size, natural_alignment = CPP_FIELDS_SIZE_AND_ALIGNMENT[field_type]
        alignment = max(natural_alignment, explicit_alignas or natural_alignment)
        
        # Align the offset
        if offset % alignment != 0:
            offset += alignment - (offset % alignment)

        cpp_struct_alignment = max(cpp_struct_alignment, alignment)
        
        cpp_fields[snake_case_to_camel_case(field_name)] = (glsl_field_type, offset)
        offset += size
    cpp_struct_alignment = max(cpp_struct_alignment, cpp_struct_alignas or cpp_struct_alignment)
    if offset % cpp_struct_alignment != 0:
        offset += cpp_struct_alignment - (offset % cpp_struct_alignment)
    cpp_struct_size = offset

    for shader in shaders:
        glsl_fields = {} # field_name -> (glsl_field_type, offset)
        glsl_fields_list = find_glsl_struct_fields(shader.read_text(), struct_name)
        offset = 0
        glsl_struct_alignment = 1
        for field_type, field_name in glsl_fields_list:
            if field_type not in GLSL_FIELDS_SIZE_AND_ALIGNMENT:
                print(f"Unsupported GLSL field type '{field_type}' in struct '{struct_name}' in shader '{shader.name}'. Skipping.")
                errors_count += 1
                continue
            
            size, alignment = GLSL_FIELDS_SIZE_AND_ALIGNMENT[field_type]
            
            # Align the offset
            if offset % alignment != 0:
                offset += alignment - (offset % alignment)

            glsl_struct_alignment = max(glsl_struct_alignment, alignment)
            
            glsl_fields[field_name] = (field_type, offset)
            offset += size
        if offset % glsl_struct_alignment != 0:
            offset += glsl_struct_alignment - (offset % glsl_struct_alignment)
        glsl_struct_size = offset
        
        if cpp_fields != glsl_fields:
            print(f"Layout mismatch for struct '{struct_name}' between C++ and shader '{shader.name}':")
            print(f"  C++ fields: {cpp_fields}")
            print(f"  GLSL fields: {glsl_fields}")
            errors_count += 1
        if cpp_struct_size != glsl_struct_size:
            print(
                f"Size mismatch for struct '{struct_name}' between C++ and shader '{shader.name}': "
                f"cpp={cpp_struct_size}, shader={glsl_struct_size}"
            )
            errors_count += 1

if errors_count != 0:
    print(f"Found {errors_count} layout mismatches.")
    exit(1)
