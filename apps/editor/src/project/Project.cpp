#include <project/Project.hpp>

#include <toml++/toml.hpp>

#include <charconv>
#include <cstdint>
#include <format>
#include <optional>
#include <sstream>
#include <system_error>

namespace TechEngine {

    std::error_code Project::load(const FileAccess& files, std::string_view manifestPath) {
        std::vector<std::byte> bytes;
        const std::error_code readError = files.read(manifestPath, bytes);
        if (readError) {
            return readError;
        }

        const std::string_view text = bytes.empty() ? std::string_view{} : std::string_view{reinterpret_cast<const char*>(bytes.data()), bytes.size()};

        const toml::parse_result parsed = toml::parse(text);
        if (!parsed) {
            return ProjectError::ParseFailed;
        }

        const std::optional<std::string> name = parsed["name"].value<std::string>();
        if (!name) {
            return ProjectError::SchemaInvalid;
        }

        std::filesystem::path physical;
        const std::error_code resolveError = files.resolve(manifestPath, physical);
        if (resolveError) {
            return resolveError;
        }

        const auto sceneNode = parsed["exportStartupScene"];
        ResourceId exportStartupScene;
        if (sceneNode) {
            const std::optional<std::string> sceneID = sceneNode.value_exact<std::string>();
            if (!sceneID || sceneID->size() != 18 || !sceneID->starts_with("0x")) {
                return ProjectError::SchemaInvalid;
            }

            std::uint64_t value = 0;
            const char* const digitsEnd = sceneID->data() + sceneID->size();
            const std::from_chars_result parsedId = std::from_chars(sceneID->data() + 2, digitsEnd, value, 16);
            if (parsedId.ec != std::errc{} || parsedId.ptr != digitsEnd) {
                return ProjectError::SchemaInvalid;
            }
            exportStartupScene = ResourceId(value);
        }

        m_root = physical.parent_path();
        m_name = *name;
        m_exportStartupScene = exportStartupScene;
        return {};
    }

    std::error_code Project::save(FileAccess& files, const std::string_view manifestPath) const {
        toml::table manifest;
        manifest.insert_or_assign("name", m_name);
        if (m_exportStartupScene.valid()) {
            manifest.insert_or_assign("exportStartupScene", std::format("{:#018x}", m_exportStartupScene.value()));
        }

        std::ostringstream stream;
        stream << manifest;
        const std::string text = stream.str();

        const std::span bytes{reinterpret_cast<const std::byte*>(text.data()), text.size()};

        const std::error_code writeError = files.write(manifestPath, bytes);
        if (writeError) {
            return writeError;
        }
        return {};
    }

    const std::filesystem::path& Project::root() const {
        return m_root;
    }

    const std::string& Project::name() const {
        return m_name;
    }

    ResourceId Project::exportStartupScene() const {
        return m_exportStartupScene;
    }

    void Project::setExportStartupScene(const ResourceId id) {
        m_exportStartupScene = id;
    }
}
