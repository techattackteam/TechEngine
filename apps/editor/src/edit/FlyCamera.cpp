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

    FlyCamera::FlyCamera() : m_position(0.0f, DEFAULT_HEIGHT, DEFAULT_DISTANCE) {
        m_pitch = glm::radians(DEFAULT_PITCH_DEGREES);
        m_targetPitch = m_pitch;
    }

    void FlyCamera::update(const FlyCameraInput& input, const float deltaTime) {
        const float step = std::clamp(deltaTime, 0.0f, MAX_DELTA_TIME);
        if (input.looking) {
            m_speedScale = std::clamp(m_speedScale * std::pow(SPEED_STEP, input.wheel), MIN_SPEED_SCALE, MAX_SPEED_SCALE);
        }

        if (m_focusing) {
            const bool steering = input.looking && (input.lookDelta != Vec2(0.0f) || input.move != Vec3(0.0f));
            if (!steering) {
                advanceFocus(step);
                return;
            }
            m_focusing = false;
        }

        look(input, step);
        move(input, step);
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
        m_velocity = Vec3(0.0f);
    }

    Mat4 FlyCamera::getWorld() const {
        return glm::translate(Mat4(1.0f), m_position) * glm::mat4_cast(getRotation());
    }

    const Vec3& FlyCamera::getPosition() const {
        return m_position;
    }

    Vec3 FlyCamera::getForward() const {
        return forwardOf(m_yaw, m_pitch);
    }

    float FlyCamera::getYaw() const {
        return m_yaw;
    }

    float FlyCamera::getPitch() const {
        return m_pitch;
    }

    const Vec3& FlyCamera::getVelocity() const {
        return m_velocity;
    }

    float FlyCamera::getSpeedScale() const {
        return m_speedScale;
    }

    bool FlyCamera::isFocusing() const {
        return m_focusing;
    }

    float FlyCamera::getTargetSpeed(const FlyCameraInput& input) const {
        float distance = DEFAULT_SPEED_DISTANCE;
        if (input.hasPivot) {
            distance = glm::length(input.pivot - m_position);
        }
        const float speed = std::clamp(distance, MIN_SPEED_DISTANCE, MAX_SPEED_DISTANCE) * SPEED_PER_METRE * m_speedScale;
        return input.fast ? speed * FAST_MULTIPLIER : speed;
    }

    void FlyCamera::look(const FlyCameraInput& input, const float deltaTime) {
        if (input.looking) {
            const float maxPitch = glm::radians(MAX_PITCH_DEGREES);
            m_targetYaw -= input.lookDelta.x * LOOK_RADIANS_PER_PIXEL;
            m_targetPitch = std::clamp(m_targetPitch - input.lookDelta.y * LOOK_RADIANS_PER_PIXEL, -maxPitch, maxPitch);
        }
        const float alpha = 1.0f - std::exp(-LOOK_SHARPNESS * deltaTime);
        m_yaw += (m_targetYaw - m_yaw) * alpha;
        m_pitch += (m_targetPitch - m_pitch) * alpha;
    }

    void FlyCamera::move(const FlyCameraInput& input, const float deltaTime) {
        Vec3 direction = Vec3(0.0f);
        if (input.looking) {
            const Quat rotation = getRotation();
            direction = rotation * Vec3(1.0f, 0.0f, 0.0f) * input.move.x + Vec3(0.0f, 1.0f, 0.0f) * input.move.y + rotation * Vec3(0.0f, 0.0f, -1.0f) * input.move.z;
            const float length = glm::length(direction);
            if (length > 1.0f) {
                direction /= length;
            }
        }

        const Vec3 targetVelocity = direction * getTargetSpeed(input);
        const float alpha = 1.0f - std::exp(-MOVE_SHARPNESS * deltaTime);
        m_velocity += (targetVelocity - m_velocity) * alpha;
        if (direction == Vec3(0.0f) && glm::length(m_velocity) < REST_SPEED) {
            m_velocity = Vec3(0.0f);
        }
        m_position += m_velocity * deltaTime;
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
