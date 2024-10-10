#pragma once


#include <cstdarg>
#include <iostream>

#include "systems/System.hpp"
#include "Layers.hpp"
#include "BPLayerInterfaceImpl.hpp"
#include "DebugRenderer.hpp"
#include "ObjectLayerPairFilterImpl.hpp"
#include "ObjectVsBroadPhaseLayerFilterImpl.hpp"
#include "MyBodyActivationListener.hpp"
#include "MyContactListener.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>


// Callback for traces, connect this to your own trace function if you have one
static void TraceImpl(const char* inFMT, ...) {
    // Format the message
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);

    // Print to the TTY
    std::cout << buffer << std::endl;
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine) {
    // Print to the TTY
    std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;

    // Breakpoint
    return true;
};

#endif // JPH_ENABLE_ASSERTS

namespace TechEngine {
    class CORE_DLL PhysicsEngine : public System {
    private:
        JPH::PhysicsSystem* physics_system;
        JPH::BodyID sphere_id;
        JPH::TempAllocatorImpl* temp_allocator;
        JPH::JobSystemThreadPool* job_system;
        MyContactListener contact_listener;
        MyBodyActivationListener body_activation_listener;

        BPLayerInterfaceImpl broad_phase_layer_interface;
        ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
        ObjectLayerPairFilterImpl object_vs_object_layer_filter;
        int step = 0;
        SystemsRegistry& m_systemsRegistry;

    public:
        DebugRenderer* debugRenderer = nullptr;

        explicit PhysicsEngine(SystemsRegistry& systemsRegistry);

        void init() override;

        void shutdown() override;

        void onUpdate() override;

        void onFixedUpdate() override;
    };
}
