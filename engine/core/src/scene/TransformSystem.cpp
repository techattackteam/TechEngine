#include "TransformSystem.hpp"

#include "Scene.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    TransformSystem::TransformSystem(SystemsRegistry& systemsRegistry): m_systemsRegistry(systemsRegistry) {
    }

    void TransformSystem::translate(Entity entity, glm::vec3 vector) {
        auto& transform = m_systemsRegistry.getSystem<Scene>().getComponent<Transform>(entity);
        transform.position += vector;
    }

    void TransformSystem::translateTo(Entity entity, glm::vec3 position) {
        auto& transform = m_systemsRegistry.getSystem<Scene>().getComponent<Transform>(entity);
        transform.position = position;
    }

    void TransformSystem::translateToWorld(Entity entity, glm::vec3 worldPosition) {
        auto& transform = m_systemsRegistry.getSystem<Scene>().getComponent<Transform>(entity);
        transform.position = worldPosition;
    }

    void TransformSystem::setRotation(Entity entity, glm::vec3 rotation) {
        auto& transform = m_systemsRegistry.getSystem<Scene>().getComponent<Transform>(entity);
        transform.rotation = rotation;
    }

    void TransformSystem::rotate(Entity entity, glm::vec3 rotation) {
        auto& transform = m_systemsRegistry.getSystem<Scene>().getComponent<Transform>(entity);
        transform.rotation += rotation;
    }

    void TransformSystem::setRotation(Entity entity, glm::quat quaternion) {
        auto& transform = m_systemsRegistry.getSystem<Scene>().getComponent<Transform>(entity);
        transform.rotation = glm::eulerAngles(quaternion);
    }

    void TransformSystem::setScale(Entity entity, glm::vec3 vector) {
        auto& transform = m_systemsRegistry.getSystem<Scene>().getComponent<Transform>(entity);
        transform.scale = vector;
    }
}
