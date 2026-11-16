#pragma once

#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/core/camera/Camera.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace TechEngine {
    struct Material;
    struct Mesh;

    struct PacketCamera {
        Mat4 world = Mat4(1.0f);
        Camera settings;
    };

    struct PacketInstance {
        Entity entity;
        Mat4 world = Mat4(1.0f);
        ResourceHandle<Mesh> mesh;
        std::uint32_t firstMaterial = 0;
        std::uint32_t materialCount = 0;
    };

    struct FramePacket {
        std::array<float, 4> clearColor{};
        bool drawTriangle = false;
        std::uint64_t tick = 0;
        Clock::TimePoint tickTime{};
        double fixedDeltaTime = 1.0 / 60.0;
        std::uint64_t timeline = 0;
        std::uint64_t sequence = 0;
        InputState input;
        bool hasCamera = false;
        PacketCamera camera;
        std::vector<PacketInstance> instances;
        std::vector<ResourceHandle<Material>> materials;
    };
}
