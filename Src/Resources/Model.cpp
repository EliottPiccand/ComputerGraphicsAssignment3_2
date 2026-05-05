#include "Resources/Model.h"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <ranges>
#include <span>
#include <stdexcept>

#include <Lib/tiny_gltf.h>

#include "Resources/ResourceLoader.h"
#include "Utils/Log.h"
#include "Utils/Path.h"
#include "Utils/Profiling.h"

using namespace resource;

#pragma region vertex

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-member-function"

#pragma clang diagnostic pop

#pragma endregion vertex

#pragma region tinygltf_callbacks

namespace
{

static glm::mat4 getNodeTransform(const tg3_node &node)
{
    glm::mat4 transform(1.0f);

    if (node.has_matrix)
    {
        const double (&m)[16] = node.matrix;
        transform = glm::mat4(glm::vec4(m[0], m[1], m[2], m[3]), glm::vec4(m[4], m[5], m[6], m[7]),
                              glm::vec4(m[8], m[9], m[10], m[11]), glm::vec4(m[12], m[13], m[14], m[15]));
    }
    else
    {
        glm::vec3 translation(node.translation[0], node.translation[1], node.translation[2]);
        glm::quat rotation(static_cast<float>(node.rotation[3]),  // w
                           static_cast<float>(node.rotation[0]),  // x
                           static_cast<float>(node.rotation[1]),  // y
                           static_cast<float>(node.rotation[2])); // z
        glm::vec3 scale(node.scale[0], node.scale[1], node.scale[2]);

        transform = glm::translate(glm::mat4(1.0f), translation);
        transform *= glm::mat4_cast(rotation);
        transform *= glm::scale(glm::mat4(1.0f), scale);
    }

    return transform;
}

struct FSContext
{
    std::filesystem::path base_dir;
};

static int32_t fs_file_exists(const char *path, uint32_t path_len, void *user_data)
{
    if (!path || path_len == 0)
        return 0;

    const auto *ctx = static_cast<FSContext *>(user_data);
    auto fullPath = ctx->base_dir / std::string(path, path_len);
    return std::filesystem::exists(fullPath) ? 1 : 0;
}

static int32_t fs_read_file(uint8_t **out_data, uint64_t *out_size, const char *path, uint32_t path_len,
                            void *user_data)
{
    if (!path || path_len == 0 || !out_data || !out_size)
        return 0;

    const auto *ctx = static_cast<FSContext *>(user_data);
    auto full_path = ctx->base_dir / std::string(path, path_len);

    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open())
        return 0;

    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    auto *data = new uint8_t[static_cast<size_t>(size)];
    file.read(reinterpret_cast<char *>(data), size);
    file.close();

    *out_data = data;
    *out_size = static_cast<uint64_t>(size);
    return 1;
}

static void fs_free_file(uint8_t *data, uint64_t size, void *user_data)
{
    (void)size;
    (void)user_data;

    delete[] data;
}

} // namespace

#pragma endregion tinygltf_callbacks

Model::Model(GLuint vertex_array, GLuint vertex_buffer, std::vector<Shape> meshes)
    : vertex_array_(vertex_array), vertex_buffer_(vertex_buffer), shapes_(std::move(meshes))
{
}

Model::~Model()
{
    for (const auto &mesh : shapes_)
    {
        glDeleteBuffers(1, &mesh.index_buffer);
    }
    glDeleteBuffers(1, &vertex_buffer_);
    glDeleteVertexArrays(1, &vertex_array_);
}

namespace
{

void processNode(const size_t node_index, const glm::mat4 &parent_transform, const tg3_model &model,
                 const std::filesystem::path &path, std::vector<VertexPBR> &vertices,
                 std::unordered_map<VertexPBR, GLuint> &vertex_map,
                 std::unordered_map<int, std::vector<IndexType>> &material_indices)
{

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-container"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"

    const std::span<const tg3_node> model_nodes(model.nodes, model.nodes_count);
    const std::span<const tg3_mesh> model_meshes(model.meshes, model.meshes_count);
    const std::span<const tg3_accessor> model_accessors(model.accessors, model.accessors_count);
    const std::span<const tg3_buffer_view> model_buffer_views(model.buffer_views, model.buffer_views_count);
    const std::span<const tg3_buffer> model_buffers(model.buffers, model.buffers_count);

    if (node_index >= model_nodes.size())
    {
        LOG_WARNING("ill formatted gltf '{}', found node index {} >= model node count, skipped",
                    relativeToExeDir(path).string(), node_index);
        return;
    }
    const auto &node = model_nodes[node_index];

    glm::mat4 node_transform = parent_transform * getNodeTransform(node);

    if (0 <= node.mesh && static_cast<size_t>(node.mesh) < model_meshes.size())
    {
        const auto &mesh = model_meshes[static_cast<size_t>(node.mesh)];

        const std::span<const tg3_primitive> primitives(mesh.primitives, mesh.primitives_count);
        for (const auto &primitive : primitives)
        {
            glm::vec3 *positions = nullptr;
            glm::vec3 *normals = nullptr;
            glm::vec2 *uvs = nullptr;
            size_t vertex_count = 0;

            const std::span<const tg3_str_int_pair> attributes(primitive.attributes, primitive.attributes_count);
            for (const auto &attribute : attributes)
            {
                if (attribute.value < 0 || model_accessors.size() <= static_cast<size_t>(attribute.value))
                {
                    LOG_WARNING("ill formatted gltf '{}', found accessor index {} >= model accessor count, skipped",
                                relativeToExeDir(path).string(), attribute.value);
                    continue;
                }

                const auto &accessor = model_accessors[static_cast<size_t>(attribute.value)];
                if (accessor.buffer_view < 0 || model_buffer_views.size() <= static_cast<size_t>(accessor.buffer_view))
                {
                    LOG_WARNING(
                        "ill formatted gltf '{}', found buffer view index {} >= model buffer view count, skipped",
                        relativeToExeDir(path).string(), accessor.buffer_view);
                    continue;
                }

                const auto &bufferView = model_buffer_views[static_cast<size_t>(accessor.buffer_view)];
                if (bufferView.buffer < 0 || model_buffers.size() <= static_cast<size_t>(bufferView.buffer))
                {
                    LOG_WARNING("ill formatted gltf '{}', found buffer index {} >= model buffer count, skipped",
                                relativeToExeDir(path).string(), bufferView.buffer);
                    continue;
                }

                const auto &buffer = model_buffers[static_cast<size_t>(bufferView.buffer)];
                const uint8_t *baseData = buffer.data.data + bufferView.byte_offset + accessor.byte_offset;

                if (tg3_str_equals_cstr(attribute.key, "POSITION"))
                {
                    positions = reinterpret_cast<glm::vec3 *>(const_cast<uint8_t *>(baseData));
                    vertex_count = accessor.count;
                }
                else if (tg3_str_equals_cstr(attribute.key, "NORMAL"))
                {
                    normals = reinterpret_cast<glm::vec3 *>(const_cast<uint8_t *>(baseData));
                }
                else if (tg3_str_equals_cstr(attribute.key, "TEXCOORD_0"))
                {
                    uvs = reinterpret_cast<glm::vec2 *>(const_cast<uint8_t *>(baseData));
                }
            }

            if (primitive.indices >= 0 && vertex_count > 0)
            {
                const auto &index_accessor = model_accessors[static_cast<size_t>(primitive.indices)];
                if (index_accessor.buffer_view < 0 ||
                    model_buffer_views.size() <= static_cast<size_t>(index_accessor.buffer_view))
                {
                    LOG_WARNING(
                        "ill formatted gltf '{}', found buffer view index {} >= model buffer view count, skipped",
                        relativeToExeDir(path).string(), index_accessor.buffer_view);
                    continue;
                }

                const auto &index_buffer_view = model_buffer_views[static_cast<size_t>(index_accessor.buffer_view)];
                if (index_buffer_view.buffer < 0 ||
                    model_buffers.size() <= static_cast<size_t>(index_buffer_view.buffer))
                {
                    LOG_WARNING(
                        "ill formatted gltf '{}', found buffer view index {} >= model buffer view count, skipped",
                        relativeToExeDir(path).string(), index_buffer_view.buffer);
                    continue;
                }

                const auto &index_buffer = model_buffers[static_cast<size_t>(index_buffer_view.buffer)];
                const uint8_t *index_data =
                    index_buffer.data.data + index_buffer_view.byte_offset + index_accessor.byte_offset;

                std::vector<IndexType> &mesh_indices = material_indices[primitive.material];
                const size_t index_count = index_accessor.count;

                if (index_accessor.component_type == GL_UNSIGNED_INT)
                {
                    const std::span<const uint32_t> indices32(reinterpret_cast<const uint32_t *>(index_data),
                                                              index_count);
                    for (const auto &original_index : indices32)
                    {
                        if (original_index >= vertex_count)
                        {
                            LOG_WARNING("ill formatted gltf '{}', found buffer buffer vertex index ({}) >= vertex "
                                        "count ({}), skipped",
                                        relativeToExeDir(path).string(), original_index, vertex_count);
                            continue;
                        }

                        VertexPBR vertex{};
                        if (positions != nullptr)
                        {
                            glm::vec4 transformed_position =
                                node_transform * glm::vec4(positions[original_index], 1.0f);
                            vertex.position = glm::vec3(transformed_position);
                        }
                        if (normals != nullptr)
                        {
                            glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(node_transform)));
                            vertex.normal = glm::normalize(normal_matrix * normals[original_index]);
                        }
                        if (uvs != nullptr)
                            vertex.uv = uvs[original_index];

                        if (vertex_map.find(vertex) == vertex_map.end())
                        {
                            vertex_map[vertex] = static_cast<GLuint>(vertices.size());
                            vertices.push_back(vertex);
                        }

                        mesh_indices.push_back(static_cast<IndexType>(vertex_map[vertex]));
                    }
                }
                else if (index_accessor.component_type == GL_UNSIGNED_SHORT)
                {
                    const std::span<const uint16_t> indices16(reinterpret_cast<const uint16_t *>(index_data),
                                                              index_count);
                    for (const auto &original_index : indices16)
                    {
                        if (original_index >= vertex_count)
                        {
                            LOG_WARNING("ill formatted gltf '{}', found buffer buffer vertex index ({}) >= vertex "
                                        "count ({}), skipped",
                                        relativeToExeDir(path).string(), original_index, vertex_count);
                            continue;
                        }

                        VertexPBR vertex{};
                        if (positions != nullptr)
                        {
                            glm::vec4 transformed_position =
                                node_transform * glm::vec4(positions[original_index], 1.0f);
                            vertex.position = glm::vec3(transformed_position);
                        }
                        if (normals != nullptr)
                        {
                            glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(node_transform)));
                            vertex.normal = glm::normalize(normal_matrix * normals[original_index]);
                        }
                        if (uvs != nullptr)
                            vertex.uv = uvs[original_index];

                        if (vertex_map.find(vertex) == vertex_map.end())
                        {
                            vertex_map[vertex] = static_cast<GLuint>(vertices.size());
                            vertices.push_back(vertex);
                        }

                        mesh_indices.push_back(static_cast<IndexType>(vertex_map[vertex]));
                    }
                }
                else
                {
                    LOG_ERROR("failed to load gltf file '{}': unsupported gltf index accessor component type: {}",
                              relativeToExeDir(path).string(), index_accessor.component_type);
                    throw std::runtime_error("model loading failed");
                }
            }
        }
    }

    const std::span<const int32_t> node_children(node.children, node.children_count);
    for (const auto &child : node_children)
    {
        if (child < 0)
        {
            LOG_WARNING("ill formatted gltf '{}', found node index {} >= model node count, skipped",
                        relativeToExeDir(path).string(), child);
            continue;
        }

        processNode(static_cast<size_t>(child), node_transform, model, path, vertices, vertex_map, material_indices);
    }
#pragma clang diagnostic pop
}

} // namespace

std::shared_ptr<Model> Model::load(const std::filesystem::path &partial_path)
{
    ProfileScope;

    const auto path = ResourceLoader::ASSETS_DIRECTORY / DIRECTORY / partial_path;

    LOG_DEBUG("loading model '{}'", relativeToExeDir(path).string());

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        LOG_ERROR("failed to open file '{}'", relativeToExeDir(path).string());
        throw std::runtime_error("file open failed");
    }

    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(static_cast<size_t>(size));
    file.read(reinterpret_cast<char *>(data.data()), size);
    file.close();

    FSContext fs_context{path.parent_path()};
    tg3_fs_callbacks fs_callbacks{};
    fs_callbacks.file_exists = fs_file_exists;
    fs_callbacks.read_file = fs_read_file;
    fs_callbacks.free_file = fs_free_file;
    fs_callbacks.user_data = &fs_context;

    tg3_parse_options parse_options{};
    parse_options.fs = fs_callbacks;
    parse_options.images_as_is = 1;

    tg3_model model = {};
    tg3_error_stack errors = {};

    const std::string base_dir_str = path.parent_path().string();
    const auto err = tg3_parse_auto(&model, &errors, data.data(), static_cast<uint64_t>(size), base_dir_str.c_str(),
                                    static_cast<uint32_t>(base_dir_str.length()), &parse_options);

    if (err != TG3_OK)
    {
        if (tg3_errors_has_error(&errors))
        {
            const uint32_t error_count = tg3_errors_count(&errors);
            LOG_ERROR("error{} occurred:", error_count <= 1 ? "" : "s");
            for (uint32_t i = 0; i < error_count; ++i)
            {
                const auto *entry = tg3_errors_get(&errors, i);
                if (entry)
                {
                    LOG_ERROR("- {}", entry->message);
                }
            }
        }
        LOG_ERROR("failed to load model '{}': {}", relativeToExeDir(path).string(), static_cast<int>(err));
        throw std::runtime_error("model parsing failed");
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-container"
#pragma clang diagnostic ignored "-Wswitch-enum"

    std::vector<std::string_view> textures;
    const auto model_directory = path.parent_path();

    const std::span<const tg3_image> images(model.images, model.images_count);
    for (const auto &image : images)
    {
        const std::string_view uri(image.uri.data, image.uri.len);
        textures.push_back(uri);
    }

    std::vector<VertexPBR> vertices;
    std::unordered_map<VertexPBR, GLuint> vertex_map;
    std::unordered_map<int, std::vector<IndexType>> material_indices;

    int32_t scene_index_int = model.default_scene;
    if (scene_index_int < 0 || model.scenes_count <= static_cast<uint32_t>(scene_index_int))
    {
        LOG_DEBUG("no default scene, using first scene");
        scene_index_int = 0;
    }
    else if (model.scenes_count < static_cast<uint32_t>(scene_index_int))
    {
        LOG_ERROR("no valid scene found in glTF model");
        throw std::runtime_error("no scene in model");
    }

    const size_t scene_index = static_cast<size_t>(scene_index_int);

    const std::span<const tg3_scene> model_scenes(model.scenes, model.scenes_count);
    const auto &scene = model_scenes[scene_index];

    const std::span<const int32_t> scene_nodes(scene.nodes, scene.nodes_count);
    for (const auto &node : scene_nodes)
    {
        if (node < 0)
        {
            LOG_WARNING("ill formatted gltf '{}', found node index {} < 0, skipped", relativeToExeDir(path).string(),
                        node);
            continue;
        }
        processNode(static_cast<size_t>(node), glm::mat4(1.0f), model, path, vertices, vertex_map, material_indices);
    }

    if (vertices.empty() || material_indices.empty())
    {
        LOG_ERROR("model has no vertices or materials!");
        throw std::runtime_error("no geometry extracted from model");
    }

    // Create VAO & VBO
    GLuint vertex_array;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(VertexPBR)), vertices.data(),
                 GL_STATIC_DRAW);

    VertexPBR::setupVertexArray();

    // Create meshes with materials
    const std::span<const tg3_material> model_materials(model.materials, model.materials_count);
    const std::span<const tg3_texture> model_textures(model.textures, model.textures_count);

    std::vector<Model::Shape> meshes;
    for (const auto &[material_index, indices] : material_indices)
    {
        GLuint index_buffer;
        glGenBuffers(1, &index_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(IndexType)),
                     indices.data(), GL_STATIC_DRAW);

        // Load material
        Material material;
        if (material_index < 0)
            continue;

        const auto &gltf_material = model_materials[static_cast<size_t>(material_index)];

        // Albedo
        material.albedo_color =
            glm::vec4(static_cast<float>(gltf_material.pbr_metallic_roughness.base_color_factor[0]),
                      static_cast<float>(gltf_material.pbr_metallic_roughness.base_color_factor[1]),
                      static_cast<float>(gltf_material.pbr_metallic_roughness.base_color_factor[2]),
                      static_cast<float>(gltf_material.pbr_metallic_roughness.base_color_factor[3]));

        if (gltf_material.pbr_metallic_roughness.base_color_texture.index >= 0)
        {
            int32_t texture_index = gltf_material.pbr_metallic_roughness.base_color_texture.index;
            if (texture_index >= 0 && static_cast<size_t>(texture_index) < model.textures_count)
            {
                size_t source_index = static_cast<size_t>(model_textures[static_cast<size_t>(texture_index)].source);
                if (source_index < textures.size())
                {
                    const auto texture_uri = textures[source_index];

                    if (!ResourceLoader::isLoaded<Texture>(texture_uri))
                    {
                        ResourceLoader::load<Texture>(texture_uri, texture_uri, Texture::Type::Albedo);
                    }
                    material.albedo_texture = ResourceLoader::get<Texture>(texture_uri);
                }
            }
        }

        // Metallic / Roughness
        material.metallic_factor = static_cast<float>(gltf_material.pbr_metallic_roughness.metallic_factor);
        material.roughness_factor = static_cast<float>(gltf_material.pbr_metallic_roughness.roughness_factor);

        if (gltf_material.pbr_metallic_roughness.metallic_roughness_texture.index >= 0)
        {
            int32_t texIdx = gltf_material.pbr_metallic_roughness.metallic_roughness_texture.index;
            if (texIdx >= 0 && static_cast<size_t>(texIdx) < model.textures_count)
            {
                size_t source_index = static_cast<size_t>(model_textures[static_cast<size_t>(texIdx)].source);
                if (source_index < textures.size())
                {
                    const auto texture_uri = textures[source_index];

                    if (!ResourceLoader::isLoaded<Texture>(texture_uri))
                    {
                        ResourceLoader::load<Texture>(texture_uri, texture_uri, Texture::Type::MetallicRoughness);
                    }
                    material.metallic_roughness_texture = ResourceLoader::get<Texture>(texture_uri);
                }
            }
        }

        // Normal Map
        if (gltf_material.normal_texture.index >= 0)
        {
            int32_t texture_index = gltf_material.normal_texture.index;
            if (texture_index >= 0 && static_cast<size_t>(texture_index) < model.textures_count)
            {
                size_t source_index = static_cast<size_t>(model_textures[static_cast<size_t>(texture_index)].source);
                if (source_index < textures.size())
                {
                    const auto texture_uri = textures[source_index];

                    if (!ResourceLoader::isLoaded<Texture>(texture_uri))
                    {
                        ResourceLoader::load<Texture>(texture_uri, texture_uri, Texture::Type::NormalMap);
                    }
                    material.normal_map = ResourceLoader::get<Texture>(texture_uri);
                }
            }
        }

        // Emissive
        material.emissive_color = Color(static_cast<float>(gltf_material.emissive_factor[0]),
                                        static_cast<float>(gltf_material.emissive_factor[1]),
                                        static_cast<float>(gltf_material.emissive_factor[2]), 1.0f);

        if (gltf_material.emissive_texture.index >= 0)
        {
            int32_t texture_index = gltf_material.emissive_texture.index;
            if (texture_index >= 0 && static_cast<size_t>(texture_index) < model.textures_count)
            {
                size_t source_index = static_cast<size_t>(model_textures[static_cast<size_t>(texture_index)].source);
                if (source_index < textures.size())
                {
                    const auto texture_uri = textures[source_index];

                    if (!ResourceLoader::isLoaded<Texture>(texture_uri))
                    {
                        ResourceLoader::load<Texture>(texture_uri, texture_uri, Texture::Type::Emissive);
                    }
                    material.emissive_texture = ResourceLoader::get<Texture>(texture_uri);
                }
            }
        }

        const std::span<const tg3_extension> material_extensions(gltf_material.ext.extensions,
                                                                 gltf_material.ext.extensions_count);
        for (const auto &&[extension_index, extension] : material_extensions | std::views::enumerate)
        {
            if (tg3_str_equals_cstr(extension.name, "KHR_materials_specular"))
            {
                LOG_WARNING("material {} uses the 'KHR_materials_specular' extension, but this is incompatible with the metallic/roughness pipeline", material_index);
            }
            else
            {
                LOG_WARNING("material {} uses an unsupported extension: '{}'", material_index, extension.name.data);
            }
        }

        meshes.push_back({index_buffer, static_cast<GLsizei>(indices.size()), material});
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Validate created meshes
    if (meshes.empty())
    {
        LOG_ERROR("no meshes were created!");
        throw std::runtime_error("no meshes created from geometry data");
    }

    for (size_t i = 0; i < meshes.size(); ++i)
    {
        LOG_TRACE("mesh {}: {} indices", i, meshes[i].index_count);
    }

    tg3_model_free(&model);

    LOG_TRACE("loaded model with {} meshes", meshes.size());

    LOG_TRACE("loaded model '{}' with:", relativeToExeDir(path).string());
    LOG_TRACE("- vertex array id {}", vertex_array);
    LOG_TRACE("- vertex buffer id {}", vertex_buffer);
    for (const auto &mesh : meshes)
    {
        LOG_TRACE("- index buffer id {}", mesh.index_buffer);
    }

    return std::make_shared<Model>(vertex_array, vertex_buffer, meshes);
#pragma clang diagnostic pop
}

constexpr const GLenum BASE_COLOR_TEXTURE_SLOT = GL_TEXTURE0;
constexpr const GLenum METALLIC_ROUGHNESS_TEXTURE_SLOT = GL_TEXTURE1;
constexpr const GLenum NORMAL_MAP_TEXTURE_SLOT = GL_TEXTURE2;
constexpr const GLenum EMISSIVE_TEXTURE_SLOT = GL_TEXTURE3;
constexpr const GLenum ENVIRONMENT_MAP_TEXTURE_SLOT = GL_TEXTURE4;

void Model::draw(std::shared_ptr<resource::Shader> shader, MaterialsOverride materials_override) const
{
    ProfileScope;
    ProfileScopeGPU("Model::draw");

    drawInner(shader, materials_override, std::nullopt);
}

void Model::drawInstanced(std::shared_ptr<resource::Shader> shader, size_t intance_count,
                          MaterialsOverride materials_override) const
{
    ProfileScope;
    ProfileScopeGPU("Model::drawInstanced");

    drawInner(shader, materials_override, intance_count);
}

void Model::drawInner(std::shared_ptr<resource::Shader> shader, MaterialsOverride materials_override,
                      std::optional<size_t> intance_count) const
{
    ProfileScope;
    ProfileScopeGPU("Model::drawInner");

    glBindVertexArray(vertex_array_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);

    const auto getTexture = [](std::weak_ptr<Texture> weak_texture, Texture::Type type) {
        auto texture = weak_texture.lock();

        if (texture != nullptr)
            return texture;

        switch (type)
        {
        case Texture::Type::Albedo:
            [[fallthrough]];
        case Texture::Type::Emissive:
            return Texture::MISSING_ALBEDO;
        case Texture::Type::MetallicRoughness:
            return Texture::MISSING_METALLIC_ROUGHNESS;
        case Texture::Type::NormalMap:
            return Texture::MISSING_NORMAL_MAP;
        }
    };

    for (const auto &&[i, mesh] : shapes_ | std::views::enumerate)
    {
        auto material = mesh.material;
        if (materials_override.contains(static_cast<size_t>(i)))
        {
            const auto &material_override = materials_override[static_cast<size_t>(i)];

            // clang-format off
            material.albedo_color               = material_override.albedo_color.has_value()                ? material_override.albedo_color.value()                : material.albedo_color               ;
            material.albedo_texture             = material_override.albedo_texture.has_value()              ? material_override.albedo_texture.value()              : material.albedo_texture             ;
            material.metallic_factor            = material_override.metallic_factor.has_value()             ? material_override.metallic_factor.value()             : material.metallic_factor            ;
            material.roughness_factor           = material_override.roughness_factor.has_value()            ? material_override.roughness_factor.value()            : material.roughness_factor           ;
            material.metallic_roughness_texture = material_override.metallic_roughness_texture.has_value()  ? material_override.metallic_roughness_texture.value()  : material.metallic_roughness_texture ;
            material.normal_map                 = material_override.normal_map.has_value()                  ? material_override.normal_map.value()                  : material.normal_map                 ;
            material.emissive_color             = material_override.emissive_color.has_value()              ? material_override.emissive_color.value()              : material.emissive_color             ;
            material.emissive_texture           = material_override.emissive_texture.has_value()            ? material_override.emissive_texture.value()            : material.emissive_texture           ;
            // clang-format on
        }

        // Albedo
        shader->setUniform("u_AlbedoColor", material.albedo_color);
        getTexture(material.albedo_texture, Texture::Type::Albedo)
            ->bind(BASE_COLOR_TEXTURE_SLOT, shader, "u_AlbedoTexture");

        // Metallic / Roughness
        shader->setUniform("u_MetallicFactor", material.metallic_factor);
        shader->setUniform("u_RoughnessFactor", material.roughness_factor);
        getTexture(material.metallic_roughness_texture, Texture::Type::MetallicRoughness)
            ->bind(METALLIC_ROUGHNESS_TEXTURE_SLOT, shader, "u_MetallicRoughnessTexture");

        // Normal Map
        getTexture(material.normal_map, Texture::Type::NormalMap)->bind(NORMAL_MAP_TEXTURE_SLOT, shader, "u_NormalMap");

        // Emissive
        shader->setUniform("u_EmissiveColor", glm::vec3(material.emissive_color));
        getTexture(material.emissive_texture, Texture::Type::Emissive)
            ->bind(EMISSIVE_TEXTURE_SLOT, shader, "u_EmissiveTexture");

        // Environment Map
        if (ResourceLoader::isLoaded<Texture>("Sky/SkyBox"))
        {
            ResourceLoader::get<Texture>("Sky/SkyBox")->bind(ENVIRONMENT_MAP_TEXTURE_SLOT, shader, "u_EnvironmentMap");
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.index_buffer);

        if (intance_count.has_value())
        {
            glDrawElementsInstanced(GL_TRIANGLES, mesh.index_count, GL_INDEX_TYPE, nullptr,
                                    static_cast<GLsizei>(intance_count.value()));
        }
        else
        {
            glDrawElements(GL_TRIANGLES, mesh.index_count, GL_INDEX_TYPE, nullptr);
        }
    }
}

void Model::bind() const
{
    assert(shapes_.size() == 1 && "Model::bind() is only made for single shape models");

    glBindVertexArray(vertex_array_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shapes_[0].index_buffer);
}

GLsizei Model::getIndexCount() const
{
    assert(shapes_.size() == 1 && "Model::getIndexCount() is only made for single shape models");
    return shapes_[0].index_count;
}
