#include <TechEngine/core/scene/components/Transform.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace TechEngine {
    bool Transform::setLocal(const TransformValues& values) {
        constexpr float epsilon = 0.0001f;
        const glm::bvec3 isScaleZero = glm::epsilonEqual(values.scale, glm::vec3(0.0f), epsilon);
        if (glm::any(isScaleZero)) {
            return false;
        }
        m_local = values;
        return true;
    }

    bool Transform::setWorld(const TransformValues& values) {
        const glm::bvec3 isScaleZero = glm::equal(values.scale, Vec3(0.0f));
        if (glm::any(isScaleZero)) {
            return false;
        }
        m_world = values;
        return true;
    }

    void Transform::updateWorldMatrix(const Mat4& parentWorldMatrix) {
        const Mat4 localMatrix = glm::translate(Mat4(1.0f), m_local.position) * glm::mat4_cast(m_local.rotation) * glm::scale(Mat4(1.0f), m_local.scale);
        m_worldMatrix = parentWorldMatrix * localMatrix;
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
