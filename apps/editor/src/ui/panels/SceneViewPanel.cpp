#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <edit/EditorSnapshot.hpp>
#include <ui/panels/SceneViewPanel.hpp>

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace TechEngine {
    static constexpr std::uint64_t SCENE_VIEW_TOKEN = VIEW_TEXTURE_TOKEN_BIT | 1;

    SceneViewPanel::SceneViewPanel() : ViewPanel("Scene", SCENE_VIEW_TOKEN) {
    }

    void SceneViewPanel::updateCamera(const EditorSnapshot& snapshot, const ImVec2& imageSize, const bool imageHovered) {
        const ImGuiIO& io = ImGui::GetIO();
        trackDrags(imageHovered);

        FlyCameraInput input;
        float selectedRadius = 0.0f;
        input.hasPivot = findSelected(snapshot, input.pivot, selectedRadius);
        input.viewHeight = imageSize.y;
        if (m_panning) {
            input.panning = true;
            input.panDelta = Vec2(io.MouseDelta.x, io.MouseDelta.y);
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        if (m_looking) {
            readLookInput(input);
        }
        if (imageHovered) {
            input.zoom = io.MouseWheel;
        }

        // Any panel may have focus, such as the hierarchy right after a click on the entity, so F is not tied to this one.
        const bool focusPressed = !io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F, false);
        if (focusPressed && input.hasPivot) {
            m_camera.focus(input.pivot, selectedRadius);
        }
        m_camera.update(input, io.DeltaTime);
    }

    void SceneViewPanel::describeCamera(ViewRequest& request) const {
        request.cameraSource = ViewCameraSource::Pose;
        request.pose.world = m_camera.getWorld();
        request.pose.settings = m_camera.getSettings();
    }

    void SceneViewPanel::trackDrags(const bool imageHovered) {
        if (!m_looking && imageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            m_looking = true;
            m_lookAnchor = ImGui::GetIO().MousePos;
        }
        if (m_looking && !ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            m_looking = false;
        }
        if (!m_panning && imageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
            m_panning = true;
        }
        if (m_panning && !ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            m_panning = false;
        }
    }

    void SceneViewPanel::readLookInput(FlyCameraInput& input) {
        const ImGuiIO& io = ImGui::GetIO();
        input.looking = true;
        input.lookDelta = Vec2(io.MouseDelta.x, io.MouseDelta.y);
        input.move = Vec3(getKeyAxis(ImGuiKey_D, ImGuiKey_A), getKeyAxis(ImGuiKey_E, ImGuiKey_Q), getKeyAxis(ImGuiKey_W, ImGuiKey_S));
        input.fast = io.KeyShift;
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
        // Puts the cursor back where the drag started, so it never stops at the screen's edge and next frame's
        // delta is measured from the anchor.
        ImGui::TeleportMousePos(m_lookAnchor);
    }

    bool SceneViewPanel::findSelected(const EditorSnapshot& snapshot, Vec3& position, float& radius) {
        if (!snapshot.active.valid()) {
            return false;
        }
        // The snapshot fills world only while the active entity's components list Transform.
        const bool hasTransform = std::any_of(snapshot.components.begin(), snapshot.components.end(), [](const ComponentRow& component) {
            return component.type == componentTypeId<Transform>();
        });
        if (!hasTransform) {
            return false;
        }
        position = snapshot.world.position;
        const Vec3& scale = snapshot.world.scale;
        radius = std::max({std::abs(scale.x), std::abs(scale.y), std::abs(scale.z)});
        return true;
    }

    float SceneViewPanel::getKeyAxis(const ImGuiKey positive, const ImGuiKey negative) {
        float axis = 0.0f;
        if (ImGui::IsKeyDown(positive)) {
            axis += 1.0f;
        }
        if (ImGui::IsKeyDown(negative)) {
            axis -= 1.0f;
        }
        return axis;
    }
}
