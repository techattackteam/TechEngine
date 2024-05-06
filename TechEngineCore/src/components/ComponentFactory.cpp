#include "ComponentFactory.hpp"

#include "TransformComponent.hpp"
#include "core/Logger.hpp"
#include "network/NetworkSync.hpp"
#include "physics/BoxColliderComponent.hpp"
#include "physics/CylinderCollider.hpp"
#include "physics/RigidBody.hpp"
#include "physics/SphereCollider.hpp"
#include "render/CameraComponent.hpp"
#include "render/MeshRendererComponent.hpp"

namespace TechEngine::ComponentFactory {
    Component* createComponent(std::string name, GameObject* gameObject, EventDispatcher& eventDispatcher) {
        if (name == typeid(TransformComponent).name()) {
            return new TransformComponent(gameObject, eventDispatcher);
        } else if (name == typeid(CameraComponent).name()) {
            return new CameraComponent(gameObject, eventDispatcher);
        } else if (name == typeid(MeshRendererComponent).name()) {
            return new MeshRendererComponent(gameObject, eventDispatcher);
        } else if (name == typeid(BoxColliderComponent).name()) {
            return new BoxColliderComponent(gameObject, eventDispatcher);
        } else if (name == typeid(CylinderCollider).name()) {
            return new CylinderCollider(gameObject, eventDispatcher);
        } else if (name == typeid(SphereCollider).name()) {
            return new SphereCollider(gameObject, eventDispatcher);
        } else if (name == typeid(RigidBody).name()) {
            return new RigidBody(gameObject, eventDispatcher);
        } else if (name == typeid(NetworkSync).name()) {
            return new NetworkSync(gameObject, eventDispatcher);
        } else {
            TE_LOGGER_ERROR("ComponentFactory: Component not found: {0}", name);
            return nullptr;
        }
    }
}
