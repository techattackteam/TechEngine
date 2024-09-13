#include "TransformSystem.hpp"

#include "Scene.hpp"
#include "ScenesManager.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Components.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    TransformSystem::TransformSystem(SystemsRegistry& systemsRegistry): m_systemsRegistry(systemsRegistry) {
    }

    void TransformSystem::translate(Transform& transform, glm::vec3 vector) {
        transform.position += vector;
    }

    void TransformSystem::translateTo(Entity entity, glm::vec3 position) {
        auto& transform = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getComponent<Transform>(entity);
        transform.position = position;
    }

    void TransformSystem::translateToWorld(Entity entity, glm::vec3 worldPosition) {
        auto& transform = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getComponent<Transform>(entity);
        transform.position = worldPosition;
    }

    void TransformSystem::setRotation(Entity entity, glm::vec3 rotation) {
        auto& transform = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getComponent<Transform>(entity);
        transform.rotation = rotation;
    }

    void TransformSystem::rotate(Transform& transform, glm::vec3 rotation) {
        transform.rotation += rotation;
    }

    void TransformSystem::setRotation(Entity entity, glm::quat quaternion) {
        auto& transform = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getComponent<Transform>(entity);
        transform.rotation = glm::eulerAngles(quaternion);
    }

    void TransformSystem::setScale(Entity entity, glm::vec3 vector) {
        auto& transform = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getComponent<Transform>(entity);
        transform.scale = vector;
    }

    glm::mat4 TransformSystem::getModelMatrix(Transform& transform) {
        glm::mat4 model = glm::translate(glm::mat4(1), transform.position) *
                          glm::mat4_cast(glm::quat(glm::radians(transform.rotation))) *
                          glm::scale(glm::mat4(1), transform.scale);

        return model;
    }
}
