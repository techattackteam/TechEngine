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
    Component* createComponent(std::string name, GameObject* gameObject) {
        if (name == typeid(TransformComponent).name()) {
            return new TransformComponent(gameObject);
        } else if (name == typeid(CameraComponent).name()) {
            return new CameraComponent(gameObject);
        } else if (name == typeid(MeshRendererComponent).name()) {
            return new MeshRendererComponent(gameObject);
        } else if (name == typeid(BoxColliderComponent).name()) {
            return new BoxColliderComponent(gameObject);
        } else if (name == typeid(CylinderCollider).name()) {
            return new CylinderCollider(gameObject);
        } else if (name == typeid(SphereCollider).name()) {
            return new SphereCollider(gameObject);
        } else if (name == typeid(RigidBody).name()) {
            return new RigidBody(gameObject);
        } else if (name == typeid(NetworkSync).name()) {
            return new NetworkSync(gameObject);
        } else {
            TE_LOGGER_ERROR("ComponentFactory: Component not found: {0}", name);
            return nullptr;
        }
    }
}
