#include <ui/widgets/PropertyTable.hpp>

#include <imgui.h>

namespace TechEngine {
    bool beginPropertyTable(const char* id, const float labelWidth) {
        if (!ImGui::BeginTable(id, 2)) {
            return false;
        }
        ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
        ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthStretch);
        return true;
    }

    void propertyRow(const char* label) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::TableSetColumnIndex(1);
    }

    void endPropertyTable() {
        ImGui::EndTable();
    }
}
