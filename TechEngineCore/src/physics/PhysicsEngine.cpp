#include "PhysicsEngine.hpp"
#include "core/Logger.hpp"
#include "core/Timer.hpp"
#include "event/EventDispatcher.hpp"
#include "scene/Scene.hpp"
#include "components/TransformComponent.hpp"
#include "components/physics/SphereCollider.hpp"
#include "components/physics/CylinderCollider.hpp"
#include "mesh/CylinderMesh.hpp"
#include <typeinfo>

namespace TechEngine {
    PhysicsEngine::PhysicsEngine() {
        if (instance != nullptr) {
            delete this;
        } else {
            instance = this;
        }
        EventDispatcher::getInstance().subscribe(RequestDeleteGameObject::eventType, [this](Event *event) {
            removeActor(((RequestDeleteGameObject *) event)->getTag());
        });
    }

    PhysicsEngine::~PhysicsEngine() {
        scene->release();
        dispatcher->release();
        physics->release();
        pvd->release();
        foundation->release();
    }

    void PhysicsEngine::init() {
        foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
        if (!foundation)
            TE_LOGGER_CRITICAL("PxCreateFoundation failed!");
        pvd = PxCreatePvd(*foundation);
        physx::PxPvdTransport *transport = physx::PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);
        if (pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL)) {
            TE_LOGGER_INFO("PVD connected!");
        } else {
            TE_LOGGER_CRITICAL("PVD connection failed!");
        }
        physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, physx::PxTolerancesScale(), true, pvd);
        if (!physics)
            TE_LOGGER_CRITICAL("PxCreatePhysics failed!");

        defaultMaterial = physics->createMaterial(0.5f, 0.5f, 0.0f);
        createScene();

        TE_LOGGER_INFO("Physics engine initialized!");
    }

    void PhysicsEngine::createScene() {
        physx::PxSceneDesc sceneDesc(physics->getTolerancesScale());
        sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
        dispatcher = physx::PxDefaultCpuDispatcherCreate(2);
        sceneDesc.cpuDispatcher = dispatcher;
        sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
        scene = physics->createScene(sceneDesc);
        if (!scene)
            TE_LOGGER_CRITICAL("createScene failed!");
    }

    void PhysicsEngine::start() {
        if (!scene)
            TE_LOGGER_CRITICAL("createScene failed!");
        for (auto &actor: actors) {
            GameObject *gameObject = Scene::getInstance().getGameObjectByTag(actor.first);
            TransformComponent *transformComponent = gameObject->getComponent<TransformComponent>();
            glm::vec3 position = transformComponent->getPosition();
            glm::quat rotation = glm::quat(glm::radians(transformComponent->getOrientation()));
            actor.second->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
            scene->addActor(*actor.second);
        }
        running = true;
    }

    void PhysicsEngine::stop() {
        scene->release();
        createScene();
        running = false;
    }

    void PhysicsEngine::onUpdate() {
        if (running) {
            updateActorPositions();
            scene->simulate(Timer::getInstance().getTPS());
            scene->fetchResults(true);
            updateGameObjectPositions();
        }
    }

    void PhysicsEngine::updateActorPositions() {
        for (auto &actor: actors) {
            TransformComponent *transformComponent = static_cast<TransformComponent *>(actor.second->userData);
            glm::vec3 position = transformComponent->getWorldPosition();
            glm::quat rotation = glm::quat(glm::radians(transformComponent->getOrientation()));
            actor.second->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
        }
    }

    void PhysicsEngine::updateGameObjectPositions() {
        for (auto &actor: actors) {
            physx::PxTransform transform = actor.second->getGlobalPose();
            TransformComponent *transformComponent = static_cast<TransformComponent *>(actor.second->userData);
            transformComponent->translateToWorld(glm::vec3(transform.p.x, transform.p.y, transform.p.z));
            transformComponent->setRotation(glm::quat(transform.q.w, transform.q.x, transform.q.y, transform.q.z));
        }
    }

    void PhysicsEngine::addCollider(Collider *collider) {
        TransformComponent &t = collider->getTransform();
        glm::vec3 position = collider->getTransform().getWorldPosition();
        glm::quat rotation = glm::quat(glm::radians(t.getOrientation()));
        physx::PxTransform transform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w));
        physx::PxTransform transformOffset(physx::PxVec3(collider->getOffset().x, -collider->getOffset().y, collider->getOffset().z));
        physx::PxGeometry *geometry = createGeometry(t, collider);
        if (actors.find(collider->getGameObject()->getTag()) != actors.end()) {
            updateActor(collider, transformOffset, *geometry);
        } else {
            addActor(collider, transform, *geometry, transformOffset);
        }
        //delete &geometry;
    }

    void PhysicsEngine::removeCollider(const std::string &gameObjectTag, Collider *collider) {
        if (actors.find(gameObjectTag) != actors.end()) {
            physx::PxRigidActor *actor = actors[gameObjectTag];
            int shapesNumber = actor->getNbShapes();
            physx::PxShape **shapes = new physx::PxShape *[shapesNumber];
            actor->getShapes(shapes, shapesNumber);
            for (int i = 0; i < shapesNumber; i++) {
                if (shapes[i]->getGeometry().getType() == physx::PxGeometryType::eBOX && typeid(*collider) == typeid(BoxColliderComponent)) {
                    actor->detachShape(*shapes[i]);
                } else if (shapes[i]->getGeometry().getType() == physx::PxGeometryType::eSPHERE && typeid(*collider) == typeid(SphereCollider)) {
                    actor->detachShape(*shapes[i]);
                } else if (shapes[i]->getGeometry().getType() == physx::PxGeometryType::eCONVEXMESH && typeid(*collider) == typeid(CylinderCollider)) {
                    actor->detachShape(*shapes[i]);
                }
            }
        }
    }

    void PhysicsEngine::changeDynamic(Collider *collider) {
        if (actors.find(collider->getGameObject()->getTag()) != actors.end()) {
            physx::PxRigidActor *actor = actors[collider->getGameObject()->getTag()];
            int shapesNumber = actor->getNbShapes();
            physx::PxShape **shapes = new physx::PxShape *[shapesNumber];
            actor->getShapes(shapes, shapesNumber);
            removeCollider(collider->getGameObject()->getTag(), collider);
            TransformComponent &t = collider->getTransform();
            glm::vec3 position = t.getWorldPosition();
            glm::quat rotation = glm::quat(glm::radians(t.getOrientation()));
            physx::PxTransform transform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w));
            physx::PxTransform transformOffset(physx::PxVec3(collider->getOffset().x, -collider->getOffset().y, collider->getOffset().z));
            addActor(collider, transform, shapes[0]->getGeometry(), transformOffset);
            for (int i = 1; i < shapesNumber; i++) {
                physx::PxShape *shape = shapes[i];
                updateActor(collider, shape->getLocalPose(), shape->getGeometry());
            }
        }
    }

    physx::PxGeometry *PhysicsEngine::createGeometry(TransformComponent &transform, Collider *collider) {
        if (BoxColliderComponent *boxCollider = dynamic_cast<BoxColliderComponent *>(collider)) {
            glm::vec3 size = transform.getScale() / glm::vec3(2, 2, 2) * boxCollider->getSize();
            return new physx::PxBoxGeometry(size.x, size.y, size.z);
        } else if (SphereCollider *sphereCollider = dynamic_cast<SphereCollider *>(collider)) {
            return new physx::PxSphereGeometry(sphereCollider->getRadius());
        } else if (CylinderCollider *cylinderCollider = dynamic_cast<CylinderCollider *>(collider)) {
            return createCylinderMesh(cylinderCollider, transform);
        } else {
            TE_LOGGER_CRITICAL("Collider type not supported!");
            return nullptr;
        }
    }

    physx::PxGeometry *PhysicsEngine::createCylinderMesh(CylinderCollider *cylinderCollider, TransformComponent &t) {
        CylinderMesh mesh = CylinderMesh();
        cylinderCollider->getHeight();
        cylinderCollider->getRadius();
        physx::PxConvexMeshDesc meshDesc;
        meshDesc.points.count = static_cast<physx::PxU32>(mesh.getVertices().size());
        meshDesc.points.stride = sizeof(physx::PxVec3);
        std::vector<physx::PxVec3> convexPoints = std::vector<physx::PxVec3>(mesh.getVertices().size());
        for (int i = 0; i < mesh.getVertices().size(); i++) {
            glm::vec3 scale = t.getScale();
            glm::vec3 position = mesh.getVertices()[i].getPosition() * scale;
            convexPoints[i] = physx::PxVec3(position.x * cylinderCollider->getRadius() * 2, position.y * cylinderCollider->getHeight(), position.z * cylinderCollider->getRadius() * 2);
        }
        meshDesc.points.data = convexPoints.data();
        meshDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

        meshDesc.indices.count = static_cast<physx::PxU32>(mesh.getIndices().size());
        meshDesc.indices.stride = sizeof(physx::PxU32);
        meshDesc.indices.data = mesh.getIndices().data();
        physx::PxTolerancesScale scale;
        physx::PxCookingParams params(scale);

        physx::PxDefaultMemoryOutputStream buf;
        physx::PxConvexMeshCookingResult::Enum result;
        physx::PxConvexMesh *convexMesh;
        if (PxCookConvexMesh(params, meshDesc, buf, &result)) {
            physx::PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
            convexMesh = physics->createConvexMesh(input);
        } else {
            TE_LOGGER_CRITICAL("Cooking cylinder failed!");
            return nullptr;
        }
        physx::PxConvexMeshGeometry *convexMeshGeometry = new physx::PxConvexMeshGeometry(convexMesh);
        return convexMeshGeometry;
    }

    void PhysicsEngine::addActor(Collider *collider, const physx::PxTransform &transform, const physx::PxGeometry &geometry, const physx::PxTransform &transformOffset) {
        physx::PxRigidActor *actor = nullptr;
        if (collider->isDynamic()) {
            actor = PxCreateDynamic(*physics, transform, geometry, *defaultMaterial, 10.0f, transformOffset);
        } else {
            actor = PxCreateStatic(*physics, transform, geometry, *defaultMaterial, transformOffset);
        }
        actor->userData = collider->getGameObject()->getComponent<TransformComponent>();
        actors[collider->getGameObject()->getTag()] = actor;
        if (geometry.getType() == physx::PxGeometryType::eCONVEXMESH) {
            physx::PxConvexMeshGeometry *convexMeshGeometry = (physx::PxConvexMeshGeometry *) &geometry;
            convexMeshGeometry->convexMesh->release();
        }
    }

    void PhysicsEngine::updateActor(Collider *collider, const physx::PxTransform &transformOffset, const physx::PxGeometry &geometry) {
        physx::PxRigidActor *actor = actors[collider->getGameObject()->getTag()];
        removeCollider(collider->getGameObject()->getTag(), collider);
        physx::PxShape *shape = physics->createShape(geometry, *defaultMaterial);
        shape->setLocalPose(transformOffset);
        actor->attachShape(*shape);
        if (geometry.getType() == physx::PxGeometryType::eCONVEXMESH) {
            auto *convexMeshGeometry = (physx::PxConvexMeshGeometry *) &geometry;
            convexMeshGeometry->convexMesh->release();
        }
        //delete &geometry;
    }

    void PhysicsEngine::removeActor(const std::string &tag) {
        if (actors.find(tag) != actors.end()) {
            GameObject *gameObject = Scene::getInstance().getGameObjectByTag(tag);
            for (std::pair<const std::basic_string<char>, Component *> pair: gameObject->getComponents()) {
                if (Collider *collider = dynamic_cast<Collider *>(pair.second)) {
                    removeCollider(gameObject->getTag(), collider);
                }
            }
            actors[gameObject->getTag()]->release();
            actors.erase(gameObject->getTag());
        }
    }

    PhysicsEngine *PhysicsEngine::getInstance() {
        if (instance == nullptr) {
            return new PhysicsEngine();
        } else {
            return instance;
        };
    }
}
