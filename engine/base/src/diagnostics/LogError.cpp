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
                    case LogError::NullSink:
                        return "log sink is null";
                    case LogError::SinkTableFull:
                        return "log sink table is full";
                    case LogError::SinkNotRegistered:
                        return "log sink is not registered";
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
