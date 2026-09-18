#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace TechEngine {
    void Transform::assignFrom(const Transform& other) noexcept {
        if (m_scene != nullptr && m_scene->ownsTransform(m_entity, this)) {
            m_local = other.m_local;
            m_scene->propagateTransformSubtree(m_entity);
            return;
        }

        m_local = other.m_local;
        m_world = other.m_world;
        m_worldMatrix = other.m_worldMatrix;
        m_scene = other.m_scene;
        m_entity = other.m_entity;
    }

    Transform& Transform::operator=(const Transform& other) {
        if (this != &other) {
            assignFrom(other);
        }
        return *this;
    }

    Transform& Transform::operator=(Transform&& other) noexcept {
        if (this != &other) {
            assignFrom(other);
        }
        return *this;
    }

    void Transform::bind(Scene& scene, const Entity entity) {
        m_scene = &scene;
        m_entity = entity;
    }

    bool Transform::setLocal(const TransformValues& values) {
        const glm::bvec3 isScaleZero = glm::equal(values.scale, Vec3(0.0f));
        if (glm::any(isScaleZero) || (m_scene != nullptr && !m_scene->ownsTransform(m_entity, this))) {
            return false;
        }

        m_local = values;
        if (m_scene != nullptr) {
            m_scene->propagateTransformSubtree(m_entity);
        }

        return true;
    }

    bool Transform::setWorld(const TransformValues& values) {
        const glm::bvec3 isScaleZero = glm::equal(values.scale, Vec3(0.0f));
        if (glm::any(isScaleZero) || m_scene == nullptr || !m_scene->ownsTransform(m_entity, this)) {
            return false;
        }

        TransformValues local;
        return m_scene->fromWorldToLocal(m_entity, values, local) && setLocal(local);
    }

    void Transform::updateWorld(const Mat4& parentWorldMatrix, const TransformValues& parentWorld) {
        const Mat4 localMatrix = glm::translate(Mat4(1.0f), m_local.position) * glm::mat4_cast(m_local.rotation) * glm::scale(Mat4(1.0f), m_local.scale);
        m_worldMatrix = parentWorldMatrix * localMatrix;
        m_world = parentWorld * m_local;
        m_world.position = Vec3(m_worldMatrix[3]);
    }

    Quat Transform::fromEulerDegrees(const Vec3& angles) {
        return glm::quat(glm::radians(angles));
    }

    Vec3 Transform::toEulerDegrees(const Quat& rotation) {
        return glm::degrees(glm::eulerAngles(rotation));
    }

    const TransformValues& Transform::getLocal() const {
        return m_local;
    }

    const TransformValues& Transform::getWorld() const {
        return m_world;
    }

    const Mat4& Transform::worldMatrix() const {
        return m_worldMatrix;
    }
}
