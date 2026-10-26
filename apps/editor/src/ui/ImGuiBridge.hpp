#pragma once

#include <TechEngine/client/render/ImGuiHook.hpp>

#include <imgui.h>
#include <imgui_threaded_rendering.h>

#include <array>
#include <cstddef>
#include <mutex>

namespace TechEngine {
    class ImGuiBridge final : public ImGuiHook {
    private:
        std::mutex m_mutex;
        // Render keeps drawing its slot while main may publish twice, so two slots are not enough.
        std::array<ImDrawDataSnapshot, 3> m_snapshots;
        // Every swap takes the lock; only render reads m_drawSlot outside it, because only render swaps it.
        std::size_t m_writeSlot = 0;
        std::size_t m_pendingSlot = 1;
        std::size_t m_drawSlot = 2;
        bool m_hasPending = false;
        bool m_drawSlotFilled = false;
        ImTextureQueue m_textures;

    public:
        ImGuiBridge();

        void beginFrame();

        void publish(ImDrawData& drawData);

        void start() override;

        void draw() override;

        void stop() override;
    };
}
