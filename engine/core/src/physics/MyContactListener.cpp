#include "MyContactListener.hpp"

#include <Jolt/Physics/Body/Body.h>


#include "core/Logger.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "scene/Scene.hpp"
#include "scene/ScenesManager.hpp"

#include "events/physics/OnTriggerEnter.hpp"
#include "events/physics/OnTriggerStay.hpp"
#include "events/physics/OnTriggerLeave.hpp"
#include "events/physics/OnCollisionEnter.hpp"
#include "events/physics/OnCollisionStay.hpp"
#include "events/physics/OnCollisionLeave.hpp"

namespace TechEngine {
    void MyContactListener::init(std::unordered_map<std::string, JPH::BodyID>* bodies,
                                 std::unordered_map<std::string, JPH::BodyID>* triggers,
                                 JPH::PhysicsSystem* physicsSystem) {
        m_bodies = bodies;
        m_triggers = triggers;
        m_physicsSystem = physicsSystem;
    }

    MyContactListener::MyContactListener(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry) {
    }

    JPH::ValidateResult MyContactListener::OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) {
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    void MyContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) {
        if (!(inBody1.GetID() < inBody2.GetID()))
            JPH_BREAKPOINT;

        if (isTrigger(inBody1) || isTrigger(inBody2)) {
            auto getEntity = [&](const JPH::BodyID& bodyID) {
                for (auto& [uuid, body]: *m_triggers) {
                    if (body == bodyID) {
                        return m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getEntityByUUID(uuid);
                    }
                }
                for (auto& [uuid, body]: *m_bodies) {
                    if (body == bodyID) {
                        return m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getEntityByUUID(uuid);
                    }
                }
                return int32_t(-2);
            };

            const int32_t entity1 = getEntity(inBody1.GetID());
            const int32_t entity2 = getEntity(inBody2.GetID());

            m_systemsRegistry.getSystem<EventDispatcher>().dispatch<OnTriggerEnter>(entity1, entity2);
        } else {
            int32_t entity1 = -2;
            int32_t entity2 = -2;
            for (auto& [uuid, body]: *m_bodies) {
                if (body == inBody1.GetID()) {
                    entity1 = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getEntityByUUID(uuid);
                }
                if (body == inBody2.GetID()) {
                    entity2 = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getEntityByUUID(uuid);
                }
                if (entity1 != -2 && entity2 != -2) {
                    break;
                }
            }
            m_systemsRegistry.getSystem<EventDispatcher>().dispatch<OnCollisionEnter>(entity1, entity2);
        }
    }

    void MyContactListener::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) {
        if (!(inBody1.GetID() < inBody2.GetID()))
            JPH_BREAKPOINT;


        if (isTrigger(inBody1) || isTrigger(inBody2)) {
            auto getEntity = [&](const JPH::BodyID& bodyID) {
                for (auto& [uuid, body]: *m_triggers) {
                    if (body == bodyID) {
                        return m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getEntityByUUID(uuid);
                    }
                }
                for (auto& [uuid, body]: *m_bodies) {
                    if (body == bodyID) {
                        return m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getEntityByUUID(uuid);
                    }
                }
                return int32_t(-2);
            };

            const int32_t entity1 = getEntity(inBody1.GetID());
            const int32_t entity2 = getEntity(inBody2.GetID());

            m_systemsRegistry.getSystem<EventDispatcher>().dispatch<OnTriggerStay>(entity1, entity2);
        } else {
            int32_t entity1 = -2;
            int32_t entity2 = -2;
            for (auto& [uuid, body]: *m_bodies) {
                if (body == inBody1.GetID()) {
                    entity1 = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getEntityByUUID(uuid);
                }
                if (body == inBody2.GetID()) {
                    entity2 = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getEntityByUUID(uuid);
                }
                if (entity1 != -2 && entity2 != -2) {
                    break;
                }
            }
            m_systemsRegistry.getSystem<EventDispatcher>().dispatch<OnCollisionStay>(entity1, entity2);
        }
    }

    void MyContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) {
        JPH::BodyID body1ID = inSubShapePair.GetBody1ID();
        JPH::BodyID body2ID = inSubShapePair.GetBody2ID();
        bool isTrigger1 = false;
        bool isTrigger2 = false;
        if (!(body1ID < body2ID))
            JPH_BREAKPOINT; //

        for (auto& [uuid, body]: *m_triggers) {
            if (body == body1ID) {
                isTrigger1 = true;
                break;
            }
            if (body == body2ID) {
                isTrigger2 = true;
                break;
            }
        }

        if (isTrigger1 || isTrigger2) {
            auto getEntity = [&](const JPH::BodyID& bodyID) {
                for (auto& [uuid, body]: *m_triggers) {
                    if (body == bodyID) {
                        return m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getEntityByUUID(uuid);
                    }
                }
                for (auto& [uuid, body]: *m_bodies) {
                    if (body == bodyID) {
                        return m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getEntityByUUID(uuid);
                    }
                }
                return int32_t(-2);
            };

            const int32_t entity1 = getEntity(body1ID);
            const int32_t entity2 = getEntity(body2ID);

            m_systemsRegistry.getSystem<EventDispatcher>().dispatch<OnTriggerLeave>(entity1, entity2);
        } else {
            int32_t entity1 = -2;
            int32_t entity2 = -2;
            for (auto& [uuid, body]: *m_bodies) {
                if (body == body1ID) {
                    entity1 = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getEntityByUUID(uuid);
                }
                if (body == body2ID) {
                    entity2 = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getEntityByUUID(uuid);
                }
                if (entity1 != -2 && entity2 != -2) {
                    break;
                }
            }
            m_systemsRegistry.getSystem<EventDispatcher>().dispatch<OnCollisionLeave>(entity1, entity2);
        }
    }

    bool MyContactListener::isTrigger(const JPH::Body& body) {
        return body.IsSensor();
    }
}
