#include <TechEngine/client/ClientError.hpp>

#include <string>

namespace TechEngine {
    namespace {
        class ClientErrorCategory final : public std::error_category {
        public:
            const char* name() const noexcept override {
                return "client";
            }

            std::string message(const int value) const override {
                switch (static_cast<ClientError>(value)) {
                    case ClientError::AlreadyStarted:
                        return "client is already started";
                    case ClientError::RendererStartFailed:
                        return "render thread failed to start";
                }
                return "unknown client error";
            }
        };
    }

    const std::error_category& clientErrorCategory() {
        static const ClientErrorCategory category;
        return category;
    }

    std::error_code make_error_code(const ClientError error) {
        return {static_cast<int>(error), clientErrorCategory()};
    }
}
