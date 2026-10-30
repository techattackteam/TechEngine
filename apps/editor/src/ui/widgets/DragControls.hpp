#pragma once

#include <TechEngine/base/math/Math.hpp>

#include <imgui.h>

#include <array>

namespace TechEngine {
    // ImGui adds a drag's motion to the value it is handed each frame, but the snapshot keeps the
    // value from before the edit until the simulation publishes. So while a drag is active it is
    // handed the value it produced last, not the snapshot's.
    bool dragScalar(const char* label, ImGuiDataType type, void* value, float speed, const char* format = nullptr);

    // An X, Y and Z drag across the available width, each behind a coloured axis letter that resets
    // that axis to resetValue when clicked. changedAxes says which axes changed:
    // comparing with the snapshot misses a drag back to a value the simulation has not applied yet.
    bool drawVec3Control(const char* id, Vec3& values, float resetValue, std::array<bool, 3>& changedAxes);
}
