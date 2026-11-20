#include <TechEngine/base/math/Math.hpp>

#include <ui/panels/SceneViewPanel.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>

namespace TechEngine {
    static constexpr std::uint64_t SCENE_VIEW_TOKEN = VIEW_TEXTURE_TOKEN_BIT | 1;
    static constexpr float DEFAULT_POSE_HEIGHT = 2.0f;
    static constexpr float DEFAULT_POSE_DISTANCE = 5.0f;
    static constexpr float DEFAULT_POSE_PITCH_DEGREES = -10.0f;

    SceneViewPanel::SceneViewPanel() : ViewPanel("Scene", SCENE_VIEW_TOKEN) {
    }

    void SceneViewPanel::describeCamera(ViewRequest& request) const {
        request.cameraSource = ViewCameraSource::Pose;
        // TODO(S11-T11): the fly camera's pose replaces the fixed one.
        const Mat4 position = glm::translate(Mat4(1.0f), Vec3{0.0f, DEFAULT_POSE_HEIGHT, DEFAULT_POSE_DISTANCE});
        request.pose.world = glm::rotate(position, glm::radians(DEFAULT_POSE_PITCH_DEGREES), Vec3{1.0f, 0.0f, 0.0f});
        request.pose.settings = Camera{};
    }
}
