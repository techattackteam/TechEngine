#include <TechEngine/platform/window/WindowError.hpp>

#include <string>

namespace TechEngine {
    namespace {
        class WindowErrorCategory final : public std::error_category {
        public:
            const char* name() const noexcept override {
                return "window";
            }

            std::string message(const int value) const override {
                switch (static_cast<WindowError>(value)) {
                    case WindowError::AlreadyOpen:
                        return "window is already open";
                    case WindowError::InvalidSize:
                        return "window size must be positive";
                    case WindowError::PlatformInitFailed:
                        return "GLFW failed to initialize";
                    case WindowError::CreationFailed:
                        return "GLFW failed to create the window";
                }
                return "unknown window error";
            }
        };
    }

    const std::error_category& windowErrorCategory() {
        static const WindowErrorCategory category;
        return category;
    }

    std::error_code make_error_code(const WindowError error) {
        return {static_cast<int>(error), windowErrorCategory()};
    }
}
