//
// Created by Jonatan Nevo on 03/03/2025.
//

#include "shader_module.h"

#include <ranges>
#include <utility>
#include <portal/core/file_system.h>
#include <portal/serialization/serialize.h>

#include "portal/application/vulkan/shaders/glsl_compiler.h"
#include "portal/application/vulkan/shaders/spirv_reflection.h"
#include "portal/core/log.h"

namespace portal::vulkan
{
/**
 * @brief Pre-compiles project shader files to include header code
 * @param source The shader file
 * @returns A byte array of the final shader
 */
inline std::vector<std::string> precompile_shader(const std::string& source)
{
    std::vector<std::string> final_file;
    std::string current_line;
    std::istringstream stream(source);

    while (std::getline(stream, current_line))
    {
        if (current_line.find("#include \"") == 0)
        {
            // Include paths are relative to the base shader directory
            auto include_path = current_line.substr(10);
            const size_t last_quote = include_path.find('\"');
            if (!include_path.empty() && last_quote != std::string::npos)
                include_path = include_path.substr(0, last_quote);

            // TODO: resolve shader path
            auto include_file = precompile_shader(filesystem::read_file_string(include_path));
            for (auto& include_file_line : include_file)
            {
                final_file.push_back(include_file_line);
            }
        }
        else
        {
            final_file.push_back(current_line);
        }
    }
    return final_file;
}

inline std::vector<uint8_t> convert_to_bytes(std::vector<std::string>& lines)
{
    std::vector<uint8_t> bytes;

    for (auto& line : lines)
    {
        line += "\n";
        std::vector<uint8_t> line_bytes(line.begin(), line.end());
        bytes.insert(bytes.end(), line_bytes.begin(), line_bytes.end());
    }

    return bytes;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


ShaderVariant::ShaderVariant(std::string&& preamble, std::vector<std::string>&& processes)
    : preamble(std::move(preamble)),
      processes(std::move(processes))
{
    update_id();
}

size_t ShaderVariant::get_id() const
{
    return id;
}

void ShaderVariant::add_definitions(const std::vector<std::string>& definitions)
{
    for (auto& definition : definitions)
    {
        add_define(definition);
    }
}

void ShaderVariant::add_define(const std::string& def)
{
    processes.push_back("D" + def);
    std::string tmp_def = def;
    // The "=" needs to turn into a space
    std::ranges::replace(tmp_def, '=', ' ');
    preamble.append("#define " + tmp_def + "\n");
    update_id();
}

void ShaderVariant::add_undefine(const std::string& undef)
{
    processes.push_back("U" + undef);
    preamble.append("#undef " + undef + "\n");
    update_id();
}

void ShaderVariant::add_runtime_array_size(const std::string& runtime_array_name, size_t size)
{
    if (!runtime_array_sizes.contains(runtime_array_name))
        runtime_array_sizes.insert({runtime_array_name, size});
    else
        runtime_array_sizes[runtime_array_name] = size;
}

void ShaderVariant::set_runtime_array_sizes(const std::unordered_map<std::string, size_t>& sizes)
{
    this->runtime_array_sizes = sizes;
}

const std::string& ShaderVariant::get_preamble() const
{
    return preamble;
}

const std::vector<std::string>& ShaderVariant::get_processes() const
{
    return processes;
}

const std::unordered_map<std::string, size_t>& ShaderVariant::get_runtime_array_sizes() const
{
    return runtime_array_sizes;
}

void ShaderVariant::clear()
{
    preamble.clear();
    processes.clear();
    runtime_array_sizes.clear();
    update_id();
}

void ShaderVariant::update_id()
{
    constexpr std::hash<std::string> hasher{};
    id = hasher(preamble);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ShaderSource::ShaderSource(const std::string& filename): filename(filename), source(filesystem::read_file_string(filename))
{
    constexpr std::hash<std::string> hasher{};
    id = hasher(std::string{source.cbegin(), source.cend()});
}

size_t ShaderSource::get_id() const
{
    return id;
}

const std::string& ShaderSource::get_filename() const
{
    return filename;
}

void ShaderSource::set_source(const std::string& source)
{
    this->source = source;
    constexpr std::hash<std::string> hasher{};
    id = hasher(std::string{this->source.cbegin(), this->source.cend()});
}

const std::string& ShaderSource::get_source() const
{
    return source;
}

void ShaderSource::serialize(Serializer& serializer) const
{
    serializer << filename;
}

ShaderSource ShaderSource::deserialize(Deserializer& deserializer)
{
    return ShaderSource{deserializer.get_value<std::string>()};
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ShaderModule::ShaderModule(
    Device& device,
    const vk::ShaderStageFlagBits stage,
    const ShaderSource& glsl_source,
    const std::string& entry_point,
    const ShaderVariant& shader_variant
): device{device},
   stage{stage},
   entry_point{entry_point}
{
    debug_name = std::format("{} [variant {:X}] [entrypoint {}]", glsl_source.get_filename(), shader_variant.get_id(), entry_point);

    // Compiling from GLSL source requires the entry point
    if (entry_point.empty())
        throw std::invalid_argument("entry point is empty");

    // Compile the GLSL source
    if (!GLSLCompiler::compile_to_spirv(stage, glsl_source, entry_point, shader_variant, spirv, info_log))
    {
        LOG_CORE_ERROR_TAG("Shader", "Shader compilation failed for shader \"{}\"", glsl_source.get_filename());
        LOG_CORE_ERROR_TAG("Shader", info_log);
        throw std::runtime_error("failed to compile shader");
    }

    const auto success = SPIRVReflection::reflect_shader_resources(stage, spirv, resources, shader_variant);
    if (!success)
        throw std::runtime_error("failed to reflect shader resources");

    constexpr std::hash<std::string> hasher{};
    id = hasher(std::string{reinterpret_cast<const char*>(spirv.data()), reinterpret_cast<const char*>(spirv.data() + spirv.size())});
}

ShaderModule::ShaderModule(ShaderModule&& other) noexcept
    : device{other.device},
      id{std::exchange(other.id, 0)},
      stage{other.stage},
      entry_point(std::exchange(other.entry_point, std::string{})),
      debug_name{std::exchange(other.debug_name, {})},
      spirv{std::exchange(other.spirv, {})},
      resources{std::exchange(other.resources, {})},
      info_log{std::exchange(other.info_log, {})}
{
    other.stage = {};
}

size_t ShaderModule::get_id() const
{
    return id;
}

vk::ShaderStageFlagBits ShaderModule::get_stage() const
{
    return stage;
}

const std::vector<ShaderResource>& ShaderModule::get_resources() const
{
    return resources;
}

const std::string& ShaderModule::get_info_log() const
{
    return info_log;
}

const std::vector<uint32_t>& ShaderModule::get_binary() const
{
    return spirv;
}

void ShaderModule::set_resource_mode(const std::string& resource_name, const ShaderResourceMode& resource_mode)
{
    const auto it = std::ranges::find_if(
        resources,
        [&resource_name](const auto& resource)
        {
            return resource_name == resource.name;
        }
    );

    if (it != resources.end())
    {
        if (resource_mode == ShaderResourceMode::Dynamic)
        {
            if (it->type == ShaderResourceType::BufferUniform || it->type == ShaderResourceType::BufferStorage)
            {
                it->mode = resource_mode;
            }
            else
            {
                LOG_CORE_WARN_TAG("Vulkan", "Resource `{}` does not support dynamic.", resource_name);
            }
        }
        else
        {
            it->mode = resource_mode;
        }
    }
    else
    {
        LOG_CORE_WARN_TAG("Vulkan", "Resource '{}' not found in shader", resource_name);
    }
}
} // portal
