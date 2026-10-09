#include <TechEngine/app/SimulationError.hpp>

#include <string>

namespace TechEngine {
    namespace {
        class SimulationErrorCategory final : public std::error_category {
        public:
            const char* name() const noexcept override {
                return "simulation";
            }

            std::string message(const int value) const override {
                switch (static_cast<SimulationError>(value)) {
                    case SimulationError::AlreadyRunning:
                        return "simulation thread is already running";
                    case SimulationError::StartupFailed:
                        return "simulation thread failed to start";
                }
                return "unknown simulation error";
            }
        };
    }

    const std::error_category& simulationErrorCategory() {
        static const SimulationErrorCategory category;
        return category;
    }

    std::error_code make_error_code(const SimulationError error) {
        return {static_cast<int>(error), simulationErrorCategory()};
    }
}
