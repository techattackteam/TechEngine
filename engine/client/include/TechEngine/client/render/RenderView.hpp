#pragma once

#include <TechEngine/client/render/FramePacket.hpp>

#include <cstdint>

namespace TechEngine {
    inline constexpr std::uint64_t VIEW_TEXTURE_TOKEN_BIT = std::uint64_t{1} << 63;

    enum class ViewCameraSource : std::uint8_t { MainCamera, Pose };

    struct ViewRequest {
        // Carries VIEW_TEXTURE_TOKEN_BIT; main passes it to ImGui::Image as the view's texture.
        std::uint64_t token = 0;
        int width = 0;
        int height = 0;
        bool visible = false;
        ViewCameraSource cameraSource = ViewCameraSource::MainCamera;
        // Read only when cameraSource is Pose.
        PacketCamera pose;
    };
}
