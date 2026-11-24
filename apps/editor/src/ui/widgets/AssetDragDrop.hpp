#pragma once

#include <span>
#include <string>
#include <vector>

namespace TechEngine {
    // Call between BeginDragDropSource and EndDragDropSource. paths must not be empty.
    void setAssetDragPayload(std::span<const std::string> paths);

    // The paths the drag in flight carries. False when nothing is dragged, or the drag carries no assets.
    bool peekAssetDrag(std::vector<std::string>& out);
}
