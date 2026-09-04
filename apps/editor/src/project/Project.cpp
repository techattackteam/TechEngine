#include <project/Project.hpp>

#include <toml++/toml.hpp>

#include <optional>
#include <sstream>

namespace TechEngine {

    ProjectResult Project::load(const FileAccess& files, std::string_view manifestPath, Project& out) {
        std::vector<std::byte> bytes;
        if (files.read(manifestPath, bytes) != FileResult::Ok) {
            return ProjectResult::ReadFailed;
        }

        const std::string_view text = bytes.empty() ? std::string_view{} : std::string_view{reinterpret_cast<const char*>(bytes.data()), bytes.size()};

        const toml::parse_result parsed = toml::parse(text);
        if (!parsed) {
            return ProjectResult::ParseFailed;
        }

        const std::optional<std::string> name = parsed["name"].value<std::string>();
        if (!name) {
            return ProjectResult::SchemaInvalid;
        }

        std::filesystem::path physical;
        if (files.resolve(manifestPath, physical) != FileResult::Ok) {
            return ProjectResult::ReadFailed;
        }

        out.m_root = physical.parent_path();
        out.m_name = *name;
        return ProjectResult::Ok;
    }

    ProjectResult Project::save(FileAccess& files, const std::string_view manifestPath) const {
        toml::table manifest;
        manifest.insert_or_assign("name", m_name);

        std::ostringstream stream;
        stream << manifest;
        const std::string text = stream.str();

        const std::span bytes{reinterpret_cast<const std::byte*>(text.data()), text.size()};

        return files.write(manifestPath, bytes) == FileResult::Ok ? ProjectResult::Ok : ProjectResult::WriteFailed;
    }

    const std::filesystem::path& Project::root() const {
        return m_root;
    }

    const std::string& Project::name() const {
        return m_name;
    }
}
