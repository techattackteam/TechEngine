#pragma once

#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/scene/Entity.hpp>

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
        Scene* m_scene = nullptr;
        Entity m_entity;

        void bind(Scene& scene, Entity entity);

        void assignFrom(const Transform& other) noexcept;

        void updateWorld(const Mat4& parentWorldMatrix, const TransformValues& parentWorld);

    public:
        static constexpr std::string_view tag = "TechEngine::Transform";

        template<typename Archive>
        friend void visit(Archive& archive, Transform& value) {
            static_assert(sizeof(TransformValues) == 40, "TransformValues changed shape; check that visit still covers every field.");

            TransformValues& local = value.m_local;
            archive.field("position.x", local.position.x);
            archive.field("position.y", local.position.y);
            archive.field("position.z", local.position.z);
            // Named fields keep the bytes independent of GLM's quaternion memory order.
            archive.field("rotation.w", local.rotation.w);
            archive.field("rotation.x", local.rotation.x);
            archive.field("rotation.y", local.rotation.y);
            archive.field("rotation.z", local.rotation.z);
            archive.field("scale.x", local.scale.x);
            archive.field("scale.y", local.scale.y);
            archive.field("scale.z", local.scale.z);
        }

        Transform() = default;
        Transform(const Transform&) = default;
        Transform(Transform&&) noexcept = default;
        Transform& operator=(const Transform& other);
        Transform& operator=(Transform&& other) noexcept;

        bool setLocal(const TransformValues& values);

        bool setWorld(const TransformValues& values);

        static Quat fromEulerDegrees(const Vec3& angles);

        static Vec3 toEulerDegrees(const Quat& rotation);

        const TransformValues& getLocal() const;

        const TransformValues& getWorld() const;

        const Mat4& worldMatrix() const;
    };
}
