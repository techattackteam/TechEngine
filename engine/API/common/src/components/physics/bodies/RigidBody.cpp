#include "common/include/components/physics/bodies/RigidBody.hpp"

#include "common/include/scene/Scene.hpp"
#include "physics/PhysicsEngine.hpp"
#include "scene/Scene.hpp"

namespace TechEngineAPI {
    RigidBody::RigidBody(Entity entity,
                         TechEngine::PhysicsEngine* physicsEngine,
                         TechEngine::Scene* scene) : m_physicsEngine(physicsEngine),
                                                     m_scene(scene),
                                                     Component(entity) {
    }

    void RigidBody::updateInternalPointer(TechEngine::Scene* scene) {
        m_rigidBody = &scene->getComponent<TechEngine::RigidBody>(m_entity);
    }

    void RigidBody::setVelocity(const glm::vec3& velocity) const {
        TechEngine::Tag& tag = m_scene->getComponent<TechEngine::Tag>(m_entity);
        m_physicsEngine->setVelocity(tag, velocity);
    }

    void RigidBody::addVelocity(const glm::vec3& velocity) const {
        TechEngine::Tag& tag = m_scene->getComponent<TechEngine::Tag>(m_entity);
        m_physicsEngine->addVelocity(tag, velocity);
    }
}
