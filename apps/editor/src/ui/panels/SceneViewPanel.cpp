#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <edit/EditorCommands.hpp>
#include <edit/assets/AssetDragPayload.hpp>
#include <edit/snapshot/EditorSnapshot.hpp>
#include <ui/panels/SceneViewPanel.hpp>
#include <ui/widgets/AssetDragDrop.hpp>

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace TechEngine {
    static constexpr std::uint64_t SCENE_VIEW_TOKEN = VIEW_TEXTURE_TOKEN_BIT | 1;

    SceneViewPanel::SceneViewPanel(EditorCommands& commands) : ViewPanel("Scene", SCENE_VIEW_TOKEN), m_commands(&commands) {
    }

    void SceneViewPanel::updateCamera(const EditorSnapshot& snapshot, const ImVec2& imageSize, const bool imageHovered) {
        const ImGuiIO& io = ImGui::GetIO();
        trackDrags(imageHovered);

        FlyCameraInput input;
        float selectedRadius = 0.0f;
        input.hasPivot = findSelected(snapshot, input.pivot, selectedRadius);
        input.viewHeight = imageSize.y;
        input.fast = io.KeyShift;
        input.slow = io.KeyCtrl;
        if (m_panning) {
            input.panning = true;
            input.panDelta = Vec2(io.MouseDelta.x, io.MouseDelta.y);
        }
        if (m_looking) {
            input.looking = true;
            input.lookDelta = Vec2(io.MouseDelta.x, io.MouseDelta.y);
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

    bool SceneViewPanel::wantsCursorLocked() const {
        return m_looking || m_panning;
    }

    bool SceneViewPanel::isCameraMoving() const {
        return m_looking || m_panning || m_camera.isMoving();
    }

    void SceneViewPanel::describeCamera(ViewRequest& request) const {
        request.cameraSource = ViewCameraSource::Pose;
        request.pose.world = m_camera.getWorld();
        request.pose.settings = m_camera.getSettings();
    }

    void SceneViewPanel::acceptDrop(const EditorSnapshot& snapshot) {
        if (!ImGui::BeginDragDropTarget()) {
            return;
        }
        std::vector<std::string> paths;
        std::vector<std::string> placeable;
        const bool dragging = peekAssetDrag(paths);
        if (dragging) {
            snapshot.collectPlaceable(paths, placeable);
        }
        if (!placeable.empty()) {
            const ImGuiPayload* accepted = ImGui::AcceptDragDropPayload(ASSET_DRAG_PAYLOAD);
            if (accepted != nullptr) {
                for (const std::string& path: placeable) {
                    m_commands->placeModel(path, Entity{});
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    void SceneViewPanel::trackDrags(const bool imageHovered) {
        if (!m_looking && imageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            m_looking = true;
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
}
