#include <edit/FlyCamera.hpp>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace TechEngine {
    static constexpr float DEFAULT_HEIGHT = 2.0f;
    static constexpr float DEFAULT_DISTANCE = 5.0f;
    static constexpr float DEFAULT_PITCH_DEGREES = -10.0f;
    static constexpr float DIRECTION_EPSILON = 0.0001f;
    static constexpr float REST_DISTANCE = 0.0001f;

    FlyCamera::FlyCamera() : m_position(0.0f, DEFAULT_HEIGHT, DEFAULT_DISTANCE) {
        m_pitch = glm::radians(DEFAULT_PITCH_DEGREES);
        m_targetPitch = m_pitch;
    }

    void FlyCamera::update(const FlyCameraInput& input, const float deltaTime) {
        const float step = std::clamp(deltaTime, 0.0f, MAX_DELTA_TIME);
        if (m_focusing) {
            if (!isSteering(input)) {
                advanceFocus(step);
                return;
            }
            m_focusing = false;
        }

        m_zoomRemaining += input.zoom * getZoomStep(input);
        look(input, step);
        pan(input);
        advanceZoom(step);
        advancePan(step);
    }

    void FlyCamera::focus(const Vec3& target, const float radius) {
        const float distance = std::max(radius * FOCUS_DISTANCE_PER_RADIUS, MIN_FOCUS_DISTANCE);
        const Vec3 toTarget = target - m_position;
        const float toTargetLength = glm::length(toTarget);
        const Vec3 direction = toTargetLength > DIRECTION_EPSILON ? toTarget / toTargetLength : getForward();

        const float horizontalLength = std::sqrt(direction.x * direction.x + direction.z * direction.z);
        const float lookYaw = horizontalLength > DIRECTION_EPSILON ? std::atan2(-direction.x, -direction.z) : m_yaw;
        const float maxPitch = glm::radians(MAX_PITCH_DEGREES);
        const float lookPitch = std::clamp(std::asin(std::clamp(direction.y, -1.0f, 1.0f)), -maxPitch, maxPitch);

        m_focusStartPosition = m_position;
        m_focusStartYaw = m_yaw;
        m_focusStartPitch = m_pitch;
        m_focusEndYaw = m_yaw + wrapAngle(lookYaw - m_yaw);
        m_focusEndPitch = lookPitch;
        // Built from the clamped pitch, so a target straight below still ends up in the middle of the view.
        m_focusEndPosition = target - forwardOf(lookYaw, lookPitch) * distance;
        m_focusElapsed = 0.0f;
        m_focusing = true;
        m_zoomRemaining = 0.0f;
        m_panRemaining = Vec3(0.0f);
    }

    Mat4 FlyCamera::getWorld() const {
        return glm::translate(Mat4(1.0f), m_position) * glm::mat4_cast(getRotation());
    }

    const Camera& FlyCamera::getSettings() const {
        return m_settings;
    }

    const Vec3& FlyCamera::getPosition() const {
        return m_position;
    }

    Vec3 FlyCamera::getForward() const {
        return forwardOf(m_yaw, m_pitch);
    }

    Vec3 FlyCamera::getRight() const {
        return getRotation() * Vec3(1.0f, 0.0f, 0.0f);
    }

    Vec3 FlyCamera::getUp() const {
        return getRotation() * Vec3(0.0f, 1.0f, 0.0f);
    }

    float FlyCamera::getYaw() const {
        return m_yaw;
    }

    float FlyCamera::getPitch() const {
        return m_pitch;
    }

    bool FlyCamera::isFocusing() const {
        return m_focusing;
    }

    bool FlyCamera::isMoving() const {
        return m_focusing || m_zoomRemaining != 0.0f || m_panRemaining != Vec3(0.0f) || m_yaw != m_targetYaw || m_pitch != m_targetPitch;
    }

    float FlyCamera::getReferenceDistance(const FlyCameraInput& input) const {
        if (!input.hasPivot) {
            return DEFAULT_REFERENCE_DISTANCE;
        }
        return std::clamp(glm::length(input.pivot - m_position), MIN_REFERENCE_DISTANCE, MAX_REFERENCE_DISTANCE);
    }

    float FlyCamera::getZoomStep(const FlyCameraInput& input) const {
        float distance = DEFAULT_REFERENCE_DISTANCE;
        if (input.hasPivot) {
            // Measured from where the zoom still under way will end, so quick steps toward the pivot shrink instead of
            // passing it.
            const Vec3 destination = m_position + getForward() * m_zoomRemaining;
            distance = std::clamp(glm::length(input.pivot - destination), MIN_REFERENCE_DISTANCE, MAX_REFERENCE_DISTANCE);
        }
        return distance * ZOOM_STEP_PER_METRE * getSpeedMultiplier(input);
    }

    float FlyCamera::getPanMetresPerPixel(const FlyCameraInput& input) const {
        if (input.viewHeight <= 0.0f) {
            return 0.0f;
        }
        const float viewHeightAtDistance = 2.0f * getReferenceDistance(input) * std::tan(glm::radians(m_settings.verticalFieldOfView) * 0.5f);
        return viewHeightAtDistance / input.viewHeight * getSpeedMultiplier(input);
    }

    float FlyCamera::getSpeedMultiplier(const FlyCameraInput& input) {
        float multiplier = 1.0f;
        if (input.fast) {
            multiplier *= FAST_MULTIPLIER;
        }
        if (input.slow) {
            multiplier *= SLOW_MULTIPLIER;
        }
        return multiplier;
    }

    bool FlyCamera::isSteering(const FlyCameraInput& input) {
        const bool looked = input.looking && input.lookDelta != Vec2(0.0f);
        const bool panned = input.panning && input.panDelta != Vec2(0.0f);
        return looked || panned || input.zoom != 0.0f;
    }

    void FlyCamera::look(const FlyCameraInput& input, const float deltaTime) {
        if (input.looking) {
            const float maxPitch = glm::radians(MAX_PITCH_DEGREES);
            const float radiansPerPixel = LOOK_RADIANS_PER_PIXEL * getSpeedMultiplier(input);
            m_targetYaw -= input.lookDelta.x * radiansPerPixel;
            m_targetPitch = std::clamp(m_targetPitch - input.lookDelta.y * radiansPerPixel, -maxPitch, maxPitch);
        }
        const float alpha = 1.0f - std::exp(-LOOK_SHARPNESS * deltaTime);
        m_yaw += (m_targetYaw - m_yaw) * alpha;
        m_pitch += (m_targetPitch - m_pitch) * alpha;
        if (std::abs(m_targetYaw - m_yaw) < LOOK_REST_RADIANS && std::abs(m_targetPitch - m_pitch) < LOOK_REST_RADIANS) {
            m_yaw = m_targetYaw;
            m_pitch = m_targetPitch;
        }
    }

    void FlyCamera::pan(const FlyCameraInput& input) {
        if (!input.panning) {
            return;
        }
        const float metresPerPixel = getPanMetresPerPixel(input);
        m_panRemaining += (getUp() * input.panDelta.y - getRight() * input.panDelta.x) * metresPerPixel;
    }

    void FlyCamera::advanceZoom(const float deltaTime) {
        const float alpha = 1.0f - std::exp(-ZOOM_SHARPNESS * deltaTime);
        float distance = m_zoomRemaining * alpha;
        if (std::abs(m_zoomRemaining - distance) < REST_DISTANCE) {
            distance = m_zoomRemaining;
        }
        m_position += getForward() * distance;
        m_zoomRemaining -= distance;
    }

    void FlyCamera::advancePan(const float deltaTime) {
        const float alpha = 1.0f - std::exp(-PAN_SHARPNESS * deltaTime);
        Vec3 offset = m_panRemaining * alpha;
        if (glm::length(m_panRemaining - offset) < REST_DISTANCE) {
            offset = m_panRemaining;
        }
        m_position += offset;
        m_panRemaining -= offset;
    }

    void FlyCamera::advanceFocus(const float deltaTime) {
        m_focusElapsed = std::min(m_focusElapsed + deltaTime, FOCUS_DURATION);
        const float t = m_focusElapsed / FOCUS_DURATION;
        const float eased = t * t * (3.0f - 2.0f * t);
        m_position = glm::mix(m_focusStartPosition, m_focusEndPosition, eased);
        m_yaw = std::lerp(m_focusStartYaw, m_focusEndYaw, eased);
        m_pitch = std::lerp(m_focusStartPitch, m_focusEndPitch, eased);
        m_targetYaw = m_yaw;
        m_targetPitch = m_pitch;
        if (m_focusElapsed >= FOCUS_DURATION) {
            m_focusing = false;
        }
    }

    Quat FlyCamera::getRotation() const {
        return glm::angleAxis(m_yaw, Vec3(0.0f, 1.0f, 0.0f)) * glm::angleAxis(m_pitch, Vec3(1.0f, 0.0f, 0.0f));
    }

    Vec3 FlyCamera::forwardOf(const float yaw, const float pitch) {
        return Vec3(-std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch));
    }

    float FlyCamera::wrapAngle(const float radians) {
        constexpr float TWO_PI = 2.0f * std::numbers::pi_v<float>;
        return radians - TWO_PI * std::round(radians / TWO_PI);
    }
}
