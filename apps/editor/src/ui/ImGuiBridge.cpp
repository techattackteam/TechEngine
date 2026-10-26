#include <TechEngine/base/diagnostics/Assert.hpp>

#include <ui/ImGuiBridge.hpp>

#include <imgui_impl_opengl3.h>

#include <utility>

namespace TechEngine {
    ImGuiBridge::ImGuiBridge() {
        m_textures.UpdateTexFunc = ImGui_ImplOpenGL3_UpdateTexture;
    }

    void ImGuiBridge::beginFrame() {
        const std::lock_guard lock{m_mutex};
        m_textures.PreNewFrame();
    }

    void ImGuiBridge::publish(ImDrawData& drawData) {
        const std::lock_guard lock{m_mutex};
        m_textures.QueueRequests(&drawData);
        m_snapshots[m_writeSlot].SnapUsingSwap(&drawData, ImGui::GetTime());
        std::swap(m_writeSlot, m_pendingSlot);
        m_hasPending = true;
    }

    void ImGuiBridge::start() {
        const bool initialized = ImGui_ImplOpenGL3_Init("#version 450");
        TE_CHECK(initialized, "imgui_impl_opengl3 failed to initialize");
        const bool created = ImGui_ImplOpenGL3_CreateDeviceObjects();
        TE_CHECK(created, "imgui_impl_opengl3 failed to create its device objects");
    }

    void ImGuiBridge::draw() {
        {
            const std::lock_guard lock{m_mutex};
            if (m_hasPending) {
                std::swap(m_drawSlot, m_pendingSlot);
                m_hasPending = false;
                m_drawSlotFilled = true;
            }
            if (!m_drawSlotFilled) {
                return;
            }
            m_textures.ProcessRequests(&m_snapshots[m_drawSlot].DrawData);
        }
        ImGui_ImplOpenGL3_RenderDrawData(&m_snapshots[m_drawSlot].DrawData);
    }

    void ImGuiBridge::stop() {
        const std::lock_guard lock{m_mutex};
        m_textures.Shutdown();
        for (ImDrawDataSnapshot& snapshot: m_snapshots) {
            snapshot.Clear();
        }
        m_hasPending = false;
        m_drawSlotFilled = false;
        ImGui_ImplOpenGL3_Shutdown();
    }
}
