#include <TechEngine/platform/files/FileError.hpp>

#include <string>

namespace TechEngine {
    namespace {
        class FileErrorCategory final : public std::error_category {
        public:
            const char* name() const noexcept override {
                return "file";
            }

            std::string message(const int value) const override {
                switch (static_cast<FileError>(value)) {
                    case FileError::InvalidPath:
                        return "invalid virtual path";
                    case FileError::NoMount:
                        return "no mount for this alias";
                    case FileError::NotFound:
                        return "file not found";
                    case FileError::IsADirectory:
                        return "path is a directory";
                    case FileError::NotADirectory:
                        return "path is not a directory";
                    case FileError::AlreadyExists:
                        return "path already exists";
                    case FileError::NotEmpty:
                        return "directory is not empty";
                    case FileError::AccessDenied:
                        return "access denied";
                    case FileError::IoError:
                        return "I/O error";
                }
                return "unknown file error";
            }
        };
    }

    const std::error_category& fileErrorCategory() {
        static const FileErrorCategory category;
        return category;
    }

    std::error_code make_error_code(const FileError error) {
        return {static_cast<int>(error), fileErrorCategory()};
    }
}
