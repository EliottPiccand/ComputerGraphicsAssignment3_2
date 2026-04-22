from pathlib import Path

BASE_DIR = Path(__file__).parent.parent
SRC_DIR = BASE_DIR / "Src"
INCLUDE_DIR = BASE_DIR / "Include"

errors = []
changes_made = False

def check_libs(path, lines):
    global changes_made

    edited = False

    new_lines = []
    for line in lines:
        if (line.startswith("#include \"glm")
        or line.startswith("#include \"GLFW")
        or line.startswith("#include \"GL")
        or line.startswith("#include \"gl")
        or line.startswith("#include \"stb")
        or line.startswith("#include \"tinygltf")):
            edited = True
            print(f"removed {line} from {path.relative_to(BASE_DIR)}")
            continue
        
        new_lines.append(line)

    if edited:
        path.write_text("\n".join(new_lines) + "\n")
        changes_made = True

for header in INCLUDE_DIR.rglob("*.h"):
    lines = header.read_text().splitlines()
    
    if lines[0] != "#pragma once":
        errors.append(f"missing include guard on header {header.relative_to(BASE_DIR)}")

    if str(header.relative_to(INCLUDE_DIR)).startswith("Lib"):
        continue

    check_libs(header, lines)

for impl in SRC_DIR.rglob("*.cpp"):
    if impl.name == "Main.cpp":
        continue

    header_name = impl.relative_to(SRC_DIR).with_suffix(".h")
    header_lines = (INCLUDE_DIR / header_name).read_text().splitlines()
    header_includes = set(filter(lambda s: s.startswith("#include"), header_lines))

    lines = impl.read_text().splitlines()
    includes = set(filter(lambda s: s.startswith("#include"), lines))
    
    expected_first_include = f"#include \"{str(header_name).replace("\\", "/")}\""
    if lines[0] != expected_first_include:
        errors.append(f"implementation {impl.relative_to(BASE_DIR)} does not include its header first: expected '{expected_first_include}' found {lines[0]}")

    edited = False
    for include in includes.intersection(header_includes):
        edited = True

        print(f"removed {include} from {impl.relative_to(BASE_DIR)}")

        new_lines = []
        for line in lines:
            if line != include:
                new_lines.append(line)

        lines = new_lines
    
    if edited:
        impl.write_text("\n".join(lines) + "\n")
        changes_made = True

    check_libs(impl, lines)

print("changes made", changes_made)

if changes_made:
    print("some files were updated, please recommit with the updated files")

if len(errors) > 0:
    print(f"{len(errors)} errors:")
    for error in errors:
        print(f"- {error}")

if changes_made or len(errors) > 0:
    exit(-1)
