#include <TechEngine/base/diagnostics/LogError.hpp>

#include <string>

namespace TechEngine {
    namespace {
        class LogErrorCategory final : public std::error_category {
        public:
            const char* name() const noexcept override {
                return "log";
            }

            std::string message(const int value) const override {
                switch (static_cast<LogError>(value)) {
                    case LogError::SinkTableFull:
                        return "log sink table is full";
                    case LogError::SinkNotRegistered:
                        return "log sink is not registered";
                    case LogError::SinkAlreadyRegistered:
                        return "log sink is already registered";
                    case LogError::InsideSink:
                        return "log sinks cannot change from inside a sink's write";
                }
                return "unknown log error";
            }
        };
    }

    const std::error_category& logErrorCategory() {
        static const LogErrorCategory category;
        return category;
    }

    std::error_code make_error_code(const LogError error) {
        return {static_cast<int>(error), logErrorCategory()};
    }
}
