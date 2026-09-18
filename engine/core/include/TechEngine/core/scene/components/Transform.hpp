#pragma once

#include <TechEngine/base/math/Math.hpp>

#include <string_view>

namespace TechEngine {
    class Scene;

    struct TransformValues {
        Vec3 position = Vec3(0.0f);
        Quat rotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
        Vec3 scale = Vec3(1.0f);

        TransformValues operator*(const TransformValues& other) const {
            TransformValues result;
            result.position = position + rotation * (scale * other.position);
            result.rotation = rotation * other.rotation;
            result.scale = scale * other.scale;
            return result;
        }
    };

    class Transform {
    private:
        friend class Scene;

        TransformValues m_local;
        TransformValues m_world;
        Mat4 m_worldMatrix = Mat4(1.0f);

    public:
        static constexpr std::string_view tag = "TechEngine::Transform";

        bool setLocal(const TransformValues& values);

        bool setWorld(const TransformValues& values);

        void updateWorldMatrix(const Mat4& parentWorldMatrix);

        static Quat fromEulerDegrees(const Vec3& angles);

        static Vec3 toEulerDegrees(const Quat& rotation);

        const TransformValues& getLocal() const;

        const TransformValues& getWorld() const;

        const Mat4& worldMatrix() const;
    };
}
