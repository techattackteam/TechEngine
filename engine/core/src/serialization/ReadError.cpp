#include <TechEngine/core/serialization/ReadError.hpp>

#include <string>

namespace TechEngine {
    namespace {
        class ReadErrorCategory final : public std::error_category {
        public:
            const char* name() const noexcept override {
                return "read";
            }

            std::string message(const int value) const override {
                switch (static_cast<ReadError>(value)) {
                    case ReadError::Truncated:
                        return "buffer ended before the read completed";
                    case ReadError::BadMagic:
                        return "blob magic does not match";
                    case ReadError::BadVersion:
                        return "blob format version is not supported";
                }
                return "unknown read error";
            }
        };
    }

    const std::error_category& readErrorCategory() {
        static const ReadErrorCategory category;
        return category;
    }

    std::error_code make_error_code(const ReadError error) {
        return {static_cast<int>(error), readErrorCategory()};
    }
}
