#include "MyBodyActivationListener.hpp"

#include "core/Logger.hpp"
#include <Jolt/Physics/Body/BodyID.h>

namespace TechEngine {
    void MyBodyActivationListener::OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) {
        TE_LOGGER_INFO("Body id: {0} activated", inBodyID.GetIndex());
    }

    void MyBodyActivationListener::OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) {
        TE_LOGGER_INFO("Body id: {0} deactivated", inBodyID.GetIndex());
    }
}
