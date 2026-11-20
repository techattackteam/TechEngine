#pragma once

#include <TechEngine/client/render/ImGuiHook.hpp>

#include <imgui.h>

#include <span>
#include <vector>

namespace TechEngine {
    struct SwappedViewCommand {
        ImDrawCmd* command = nullptr;
        ImTextureID token = ImTextureID_Invalid;
    };

    class ViewTextureSwap {
    private:
        std::vector<SwappedViewCommand> m_swapped;

    public:
        void apply(ImDrawData& drawData, std::span<const ImGuiViewTexture> viewTextures);

        void restore();
    };
}
