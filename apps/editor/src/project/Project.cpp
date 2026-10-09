#include <project/Project.hpp>

#include <toml++/toml.hpp>

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

        m_root = physical.parent_path();
        m_name = *name;
        return {};
    }

    std::error_code Project::save(FileAccess& files, const std::string_view manifestPath) const {
        toml::table manifest;
        manifest.insert_or_assign("name", m_name);

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
}
