#include "MyBodyActivationListener.hpp"
#include "core/Logger.hpp"

namespace TechEngine {
    void MyBodyActivationListener::OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) {
        TE_LOGGER_INFO("Body activated");
    }

    void MyBodyActivationListener::OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) {
        TE_LOGGER_INFO("Body deactivated");
    }
}
