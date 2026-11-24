#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace TechEngine {
    inline constexpr const char* ASSET_DRAG_PAYLOAD = "TE_ASSET_PATHS";

    void writeAssetDragPayload(std::span<const std::string> paths, std::string& out);

    bool readAssetDragPayload(std::string_view bytes, std::vector<std::string>& out);
}
