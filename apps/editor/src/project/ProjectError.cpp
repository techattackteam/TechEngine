#include <project/ProjectError.hpp>

#include <string>

namespace TechEngine {
    namespace {
        class ProjectErrorCategory final : public std::error_category {
        public:
            const char* name() const noexcept override {
                return "project";
            }

            std::string message(const int value) const override {
                switch (static_cast<ProjectError>(value)) {
                    case ProjectError::ParseFailed:
                        return "project manifest is not valid TOML";
                    case ProjectError::SchemaInvalid:
                        return "project manifest does not match its schema";
                    case ProjectError::NoStartupScene:
                        return "export target names no startup scene";
                    case ProjectError::UnknownStartupScene:
                        return "export target's startup scene is not an indexed asset";
                    case ProjectError::StartupSceneNotInTarget:
                        return "export target's startup scene is on a side the target does not ship";
                    case ProjectError::UnknownReference:
                        return "export target's startup scene references an asset that is not indexed";
                }
                return "unknown project error";
            }
        };
    }

    const std::error_category& projectErrorCategory() {
        static const ProjectErrorCategory category;
        return category;
    }

    std::error_code make_error_code(const ProjectError error) {
        return {static_cast<int>(error), projectErrorCategory()};
    }
}
