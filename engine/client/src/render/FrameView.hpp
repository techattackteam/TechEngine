#pragma once

#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/client/render/FramePacket.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>

#include <array>
#include <vector>

namespace TechEngine {
    struct FrameView {
        // Points into the history, so the next acquire can free it.
        const FramePacket* packet = nullptr;
        std::array<float, 4> clearColor{};
        InputState input;
        bool hasCamera = false;
        PacketCamera camera;
        // One per packet->instances entry, in the same order.
        std::vector<Mat4> instanceWorlds;
    };
}
