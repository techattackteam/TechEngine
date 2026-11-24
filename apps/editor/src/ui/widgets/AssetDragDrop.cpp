#include <edit/assets/AssetDragPayload.hpp>
#include <ui/widgets/AssetDragDrop.hpp>

#include <imgui.h>

#include <cstddef>
#include <string_view>

namespace TechEngine {
    void setAssetDragPayload(const std::span<const std::string> paths) {
        std::string bytes;
        writeAssetDragPayload(paths, bytes);
        // ImGui copies the bytes, so they need not outlive this call.
        ImGui::SetDragDropPayload(ASSET_DRAG_PAYLOAD, bytes.data(), bytes.size());
    }

    bool peekAssetDrag(std::vector<std::string>& out) {
        out.clear();
        const ImGuiPayload* payload = ImGui::GetDragDropPayload();
        if (payload == nullptr || !payload->IsDataType(ASSET_DRAG_PAYLOAD) || payload->DataSize <= 0) {
            return false;
        }
        const std::string_view bytes(static_cast<const char*>(payload->Data), static_cast<std::size_t>(payload->DataSize));
        return readAssetDragPayload(bytes, out);
    }
}
