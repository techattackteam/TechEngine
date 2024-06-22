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
    Component* createComponent(std::string name, GameObject* gameObject, SystemsRegistry& systemsRegistry) {
        if (name == typeid(TransformComponent).name()) {
            return new TransformComponent(gameObject, systemsRegistry);
        } else if (name == typeid(CameraComponent).name()) {
            return new CameraComponent(gameObject, systemsRegistry);
        } else if (name == typeid(MeshRendererComponent).name()) {
            return new MeshRendererComponent(gameObject, systemsRegistry);
        } else if (name == typeid(BoxColliderComponent).name()) {
            return new BoxColliderComponent(gameObject, systemsRegistry);
        } else if (name == typeid(CylinderCollider).name()) {
            return new CylinderCollider(gameObject, systemsRegistry);
        } else if (name == typeid(SphereCollider).name()) {
            return new SphereCollider(gameObject, systemsRegistry);
        } else if (name == typeid(RigidBody).name()) {
            return new RigidBody(gameObject, systemsRegistry);
        } else if (name == typeid(NetworkSync).name()) {
            return new NetworkSync(gameObject, systemsRegistry);
        } else {
            TE_LOGGER_ERROR("ComponentFactory: Component not found: {0}", name);
            return nullptr;
        }
    }
}
