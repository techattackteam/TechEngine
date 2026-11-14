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
                    case ProjectError::SourceUnreadable:
                        return "source file is not a model the importer can read";
                    case ProjectError::SourceHasNoMeshes:
                        return "source file has no mesh made of triangles";
                    case ProjectError::ImportFolderExists:
                        return "import folder already exists";
                    case ProjectError::SourceCompressed:
                        return "source file uses a quantized or meshopt-compressed mesh extension";
                    case ProjectError::InvalidImportScale:
                        return "import scale is not a finite number above zero";
                    case ProjectError::UnsupportedImageType:
                        return "source file is not an image type the importer reads";
                    case ProjectError::SourceNotInAssets:
                        return "source file is not under a side folder of the assets folder";
                    case ProjectError::ImageUnreadable:
                        return "source file is not an image the decoder can read";
                    case ProjectError::ImportOutputExists:
                        return "import output already exists";
                    case ProjectError::UnsupportedDdsFormat:
                        return "DDS file holds a pixel format the importer does not decode";
                    case ProjectError::ReimportTargetMissing:
                        return "re-import found no earlier import of the source";
                    case ProjectError::ReimportRootMissing:
                        return "import folder holds no root asset to take the settings from";
                    case ProjectError::EditorNotStopped:
                        return "re-import runs only while the editor is Stopped";
                    case ProjectError::ReimportInFlight:
                        return "a re-import of the source is already running";
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
