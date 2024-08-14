#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

namespace TechEngine {
    class MyBodyActivationListener : public JPH::BodyActivationListener {
    public:
        void OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override;

        void OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override;
    };
}
