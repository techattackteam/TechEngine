#pragma once

#include <TechEngine/base/math/Math.hpp>

namespace TechEngine {
    struct FlyCameraInput {
        bool looking = false;
        // Screen pixels since the last update, x to the right and y down.
        Vec2 lookDelta = Vec2(0.0f);
        // Each axis from -1 to 1: x to the camera's right, y along the world's up, z to the camera's front.
        Vec3 move = Vec3(0.0f);
        bool fast = false;
        // Wheel steps; each one scales the speed by SPEED_STEP.
        float wheel = 0.0f;
        // Speed is proportional to the distance to this point, such as the selected entity.
        bool hasPivot = false;
        Vec3 pivot = Vec3(0.0f);
    };

    class FlyCamera {
    private:
        Vec3 m_position = Vec3(0.0f);
        float m_yaw = 0.0f;
        float m_pitch = 0.0f;
        float m_targetYaw = 0.0f;
        float m_targetPitch = 0.0f;
        Vec3 m_velocity = Vec3(0.0f);
        float m_speedScale = 1.0f;
        bool m_focusing = false;
        float m_focusElapsed = 0.0f;
        Vec3 m_focusStartPosition = Vec3(0.0f);
        Vec3 m_focusEndPosition = Vec3(0.0f);
        float m_focusStartYaw = 0.0f;
        float m_focusEndYaw = 0.0f;
        float m_focusStartPitch = 0.0f;
        float m_focusEndPitch = 0.0f;

    public:
        static constexpr float LOOK_RADIANS_PER_PIXEL = 0.0025f;
        static constexpr float LOOK_SHARPNESS = 30.0f;
        static constexpr float MOVE_SHARPNESS = 10.0f;
        static constexpr float MAX_PITCH_DEGREES = 89.0f;
        static constexpr float SPEED_PER_METRE = 1.0f;
        static constexpr float MIN_SPEED_DISTANCE = 1.0f;
        static constexpr float MAX_SPEED_DISTANCE = 2000.0f;
        static constexpr float DEFAULT_SPEED_DISTANCE = 5.0f;
        static constexpr float FAST_MULTIPLIER = 3.0f;
        static constexpr float SPEED_STEP = 1.2f;
        static constexpr float MIN_SPEED_SCALE = 0.05f;
        static constexpr float MAX_SPEED_SCALE = 20.0f;
        static constexpr float FOCUS_DURATION = 0.4f;
        static constexpr float FOCUS_DISTANCE_PER_RADIUS = 2.5f;
        static constexpr float MIN_FOCUS_DISTANCE = 1.5f;
        static constexpr float MAX_DELTA_TIME = 0.1f;
        static constexpr float REST_SPEED = 0.001f;

        FlyCamera();

        void update(const FlyCameraInput& input, float deltaTime);

        // Flies to look at target from far enough to frame a sphere of this radius. Looking or moving cancels it.
        void focus(const Vec3& target, float radius);

        Mat4 getWorld() const;

        const Vec3& getPosition() const;

        Vec3 getForward() const;

        float getYaw() const;

        float getPitch() const;

        const Vec3& getVelocity() const;

        float getSpeedScale() const;

        bool isFocusing() const;

        // The speed that update() moves toward for an input on one axis.
        float getTargetSpeed(const FlyCameraInput& input) const;

    private:
        void look(const FlyCameraInput& input, float deltaTime);

        void move(const FlyCameraInput& input, float deltaTime);

        void advanceFocus(float deltaTime);

        Quat getRotation() const;

        static Vec3 forwardOf(float yaw, float pitch);

        static float wrapAngle(float radians);
    };
}
