#include "ProfilerPanel.hpp"

#include "core/Timer.hpp"
#include "profiling/Profiler.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    ProfilerPanel::ProfilerPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry) : Panel(editorSystemsRegistry), m_appSystemsRegistry(appSystemsRegistry) {
    }

    void ProfilerPanel::onInit() {
    }

    void ProfilerPanel::onUpdate() {
        // --- Get Timers and Data ---
        Timer& editorTimer = m_editorSystemsRegistry.getSystem<Timer>();
        Timer& appTimer = m_appSystemsRegistry.getSystem<Timer>(); // This is the client/server timer
        const auto& profilingData = m_appSystemsRegistry.getSystem<Profiler>().getProfilingData(); // Get our new data!

        float totalFrameTime = appTimer.getFrameTimeMS();

        // --- Basic Stats ---
        ImGui::Text("App FPS: %d", appTimer.getFPS());
        ImGui::Text("App Frame Time: %.3f ms", totalFrameTime);
        ImGui::Separator();
        ImGui::Text("Editor FPS: %d", editorTimer.getFPS());
        ImGui::Text("Editor Frame Time: %.3f ms", editorTimer.getFrameTimeMS());
        ImGui::Separator();

        // --- Per-System Breakdown ---
        if (ImGui::CollapsingHeader("System Timings", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("profiling", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("System Name");
                ImGui::TableSetupColumn("Update Time (ms)");
                ImGui::TableSetupColumn("Fixed Update Time (ms)");
                ImGui::TableHeadersRow();

                float totalUpdateTime = 0.0f;
                float totalFixedTime = 0.0f;

                for (const auto& pair: profilingData) {
                    const auto& data = pair.second;
                    totalUpdateTime += data.avgUpdateTimeMs;
                    totalFixedTime += data.avgFixedUpdateTimeMs;

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", data.systemName.c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.4f", data.avgUpdateTimeMs);

                    // Add a simple progress bar to visualize the cost
                    if (totalFrameTime > 0.0f) {
                        ImGui::SameLine();
                        ImGui::ProgressBar(data.avgUpdateTimeMs / totalFrameTime, ImVec2(-1.0f, 0.0f), "");
                    }


                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%.4f", data.avgFixedUpdateTimeMs);
                }

                // --- Footer for Totals ---
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Total Monitored Time");

                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.4f", totalUpdateTime);

                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.4f", totalFixedTime);

                ImGui::EndTable();
            }
        }
    }
}
