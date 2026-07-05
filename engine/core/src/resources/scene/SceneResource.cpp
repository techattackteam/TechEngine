#include "SceneResource.hpp"

#include "scene/SceneInternal.hpp"
#include "TechEngine/core/scene/Scene.hpp"

namespace TechEngine {
    SceneResource::SceneResource(const std::string& name, const std::filesystem::path& path) : IResource(name), m_virtualPath(path) {
    }

    const std::filesystem::path& SceneResource::getVirtualPath() const {
        return m_virtualPath;
    }

    void SceneResource::setPath(const std::filesystem::path& path) {
        this->m_virtualPath = path;
    }

    void SceneResource::serialize(StreamWriter* writer, const SceneResource& resource, const Scene& scene) {
        writer->writeString(resource.m_name);
        writer->writeString(resource.m_virtualPath.string());

        uint32_t entityCount = 0;
        for (const auto& archetype: scene.m_archetypesManager.m_archetypes) {
            entityCount += static_cast<uint32_t>(archetype->m_entities.size());
        }
        writer->writeRaw(entityCount);

        for (const auto& archetype: scene.m_archetypesManager.m_archetypes) {
            for (size_t i = 0; i < archetype->m_entities.size(); i++) {
                Entity entity = archetype->m_entities[i];
                writer->writeRaw(entity);

                uint32_t componentCount = static_cast<uint32_t>(archetype->m_componentTypes.size());
                writer->writeRaw(componentCount);

                for (const ComponentTypeID& typeID: archetype->m_componentTypes) {
                    writer->writeRaw(typeID);

                    if (typeID == ComponentType<Tag>::get()) {
                        auto* storage = static_cast<ComponentStorage<Tag>*>(archetype->m_componentData.at(typeID).get());
                        Tag::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<Hierarchy>::get()) {
                        auto* storage = static_cast<ComponentStorage<Hierarchy>*>(archetype->m_componentData.at(typeID).get());
                        Hierarchy::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<Transform>::get()) {
                        auto* storage = static_cast<ComponentStorage<Transform>*>(archetype->m_componentData.at(typeID).get());
                        Transform::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<Camera>::get()) {
                        auto* storage = static_cast<ComponentStorage<Camera>*>(archetype->m_componentData.at(typeID).get());
                        Camera::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<MeshRenderer>::get()) {
                        auto* storage = static_cast<ComponentStorage<MeshRenderer>*>(archetype->m_componentData.at(typeID).get());
                        MeshRenderer::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<PointLight>::get()) {
                        auto* storage = static_cast<ComponentStorage<PointLight>*>(archetype->m_componentData.at(typeID).get());
                        PointLight::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<DirectionalLight>::get()) {
                        auto* storage = static_cast<ComponentStorage<DirectionalLight>*>(archetype->m_componentData.at(typeID).get());
                        DirectionalLight::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<SpotLight>::get()) {
                        auto* storage = static_cast<ComponentStorage<SpotLight>*>(archetype->m_componentData.at(typeID).get());
                        SpotLight::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<StaticBody>::get()) {
                        auto* storage = static_cast<ComponentStorage<StaticBody>*>(archetype->m_componentData.at(typeID).get());
                        StaticBody::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<KinematicBody>::get()) {
                        auto* storage = static_cast<ComponentStorage<KinematicBody>*>(archetype->m_componentData.at(typeID).get());
                        KinematicBody::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<RigidBody>::get()) {
                        auto* storage = static_cast<ComponentStorage<RigidBody>*>(archetype->m_componentData.at(typeID).get());
                        RigidBody::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<BoxCollider>::get()) {
                        auto* storage = static_cast<ComponentStorage<BoxCollider>*>(archetype->m_componentData.at(typeID).get());
                        BoxCollider::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<SphereCollider>::get()) {
                        auto* storage = static_cast<ComponentStorage<SphereCollider>*>(archetype->m_componentData.at(typeID).get());
                        SphereCollider::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<CapsuleCollider>::get()) {
                        auto* storage = static_cast<ComponentStorage<CapsuleCollider>*>(archetype->m_componentData.at(typeID).get());
                        CapsuleCollider::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<CylinderCollider>::get()) {
                        auto* storage = static_cast<ComponentStorage<CylinderCollider>*>(archetype->m_componentData.at(typeID).get());
                        CylinderCollider::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<BoxTrigger>::get()) {
                        auto* storage = static_cast<ComponentStorage<BoxTrigger>*>(archetype->m_componentData.at(typeID).get());
                        BoxTrigger::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<SphereTrigger>::get()) {
                        auto* storage = static_cast<ComponentStorage<SphereTrigger>*>(archetype->m_componentData.at(typeID).get());
                        SphereTrigger::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<CapsuleTrigger>::get()) {
                        auto* storage = static_cast<ComponentStorage<CapsuleTrigger>*>(archetype->m_componentData.at(typeID).get());
                        CapsuleTrigger::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<CylinderTrigger>::get()) {
                        auto* storage = static_cast<ComponentStorage<CylinderTrigger>*>(archetype->m_componentData.at(typeID).get());
                        CylinderTrigger::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<AudioListener>::get()) {
                        auto* storage = static_cast<ComponentStorage<AudioListener>*>(archetype->m_componentData.at(typeID).get());
                        AudioListener::serialize(writer, storage->get(i));
                    } else if (typeID == ComponentType<AudioEmitter>::get()) {
                        auto* storage = static_cast<ComponentStorage<AudioEmitter>*>(archetype->m_componentData.at(typeID).get());
                        AudioEmitter::serialize(writer, storage->get(i));
                    }
                }
            }
        }
    }

    std::shared_ptr<SceneResource> SceneResource::deserializeMetadata(StreamReader* reader) {
        if (!reader->isStreamGood()) {
            return nullptr;
        }
        std::string name;
        reader->readString(name);

        std::string scenePath;
        reader->readString(scenePath);

        return std::make_shared<SceneResource>(name, std::filesystem::path(scenePath));
    }

    std::shared_ptr<SceneResource> SceneResource::deserialize(StreamReader* reader, Scene& scene, PhysicsEngine& physicsEngine) {
        if (!reader->isStreamGood()) {
            return nullptr;
        }

        std::shared_ptr<SceneResource> resource = deserializeMetadata(reader);
        scene.getInternal()->setName(resource->m_name);

        uint32_t entityCount = 0;
        reader->readRaw(entityCount);

        for (uint32_t e = 0; e < entityCount; e++) {
            Entity entity;
            reader->readRaw(entity);

            scene.m_archetypesManager.registerEntity(entity);

            uint32_t componentCount = 0;
            reader->readRaw(componentCount);

            Tag tag;
            Transform transform;
            bool hasTag = false;
            bool hasTransform = false;

            for (uint32_t c = 0; c < componentCount; c++) {
                ComponentTypeID typeID;
                reader->readRaw(typeID);

                if (typeID == ComponentType<Tag>::get()) {
                    tag = Tag::deserialize(reader);
                    hasTag = true;
                    scene.addComponent(entity, tag);
                } else if (typeID == ComponentType<Hierarchy>::get()) {
                    Hierarchy hierarchy = Hierarchy::deserialize(reader);
                    scene.addComponent(entity, hierarchy);
                } else if (typeID == ComponentType<Transform>::get()) {
                    transform = Transform::deserialize(reader);
                    hasTransform = true;
                    scene.addComponent(entity, transform);
                } else if (typeID == ComponentType<Camera>::get()) {
                    Camera camera = Camera::deserialize(reader);
                    scene.addComponent(entity, camera);
                } else if (typeID == ComponentType<MeshRenderer>::get()) {
                    MeshRenderer meshRenderer = MeshRenderer::deserialize(reader);
                    scene.addComponent(entity, meshRenderer);
                } else if (typeID == ComponentType<PointLight>::get()) {
                    PointLight pointLight = PointLight::deserialize(reader);
                    scene.addComponent(entity, pointLight);
                } else if (typeID == ComponentType<DirectionalLight>::get()) {
                    DirectionalLight directionalLight = DirectionalLight::deserialize(reader);
                    scene.addComponent(entity, directionalLight);
                } else if (typeID == ComponentType<SpotLight>::get()) {
                    SpotLight spotLight = SpotLight::deserialize(reader);
                    scene.addComponent(entity, spotLight);
                } else if (typeID == ComponentType<StaticBody>::get()) {
                    StaticBody staticBody = StaticBody::deserialize(reader, physicsEngine, tag, transform);
                    scene.addComponent(entity, staticBody);
                } else if (typeID == ComponentType<KinematicBody>::get()) {
                    KinematicBody kinematicBody = KinematicBody::deserialize(reader, physicsEngine, tag, transform);
                    scene.addComponent(entity, kinematicBody);
                } else if (typeID == ComponentType<RigidBody>::get()) {
                    RigidBody rigidBody = RigidBody::deserialize(reader, physicsEngine, tag, transform);
                    scene.addComponent(entity, rigidBody);
                } else if (typeID == ComponentType<BoxCollider>::get()) {
                    BoxCollider boxCollider = BoxCollider::deserialize(reader, physicsEngine, tag, transform);
                    scene.addComponent(entity, boxCollider);
                } else if (typeID == ComponentType<SphereCollider>::get()) {
                    SphereCollider sphereCollider = SphereCollider::deserialize(reader, physicsEngine, tag, transform);
                    scene.addComponent(entity, sphereCollider);
                } else if (typeID == ComponentType<CapsuleCollider>::get()) {
                    CapsuleCollider capsuleCollider = CapsuleCollider::deserialize(reader, physicsEngine, tag, transform);
                    scene.addComponent(entity, capsuleCollider);
                } else if (typeID == ComponentType<CylinderCollider>::get()) {
                    CylinderCollider cylinderCollider = CylinderCollider::deserialize(reader, physicsEngine, tag, transform);
                    scene.addComponent(entity, cylinderCollider);
                } else if (typeID == ComponentType<BoxTrigger>::get()) {
                    BoxTrigger boxTrigger = BoxTrigger::deserialize(reader, physicsEngine, tag, transform);
                    scene.addComponent(entity, boxTrigger);
                } else if (typeID == ComponentType<SphereTrigger>::get()) {
                    SphereTrigger sphereTrigger = SphereTrigger::deserialize(reader, physicsEngine, tag, transform);
                    scene.addComponent(entity, sphereTrigger);
                } else if (typeID == ComponentType<CapsuleTrigger>::get()) {
                    CapsuleTrigger capsuleTrigger = CapsuleTrigger::deserialize(reader, physicsEngine, tag, transform);
                    scene.addComponent(entity, capsuleTrigger);
                } else if (typeID == ComponentType<CylinderTrigger>::get()) {
                    CylinderTrigger cylinderTrigger = CylinderTrigger::deserialize(reader, physicsEngine, tag, transform);
                    scene.addComponent(entity, cylinderTrigger);
                } else if (typeID == ComponentType<AudioListener>::get()) {
                    AudioListener audioListener = AudioListener::deserialize(reader);
                    scene.addComponent(entity, audioListener);
                } else if (typeID == ComponentType<AudioEmitter>::get()) {
                    AudioEmitter audioEmitter = AudioEmitter::deserialize(reader);
                    scene.addComponent(entity, audioEmitter);
                }
            }
        }
        return resource;
    }
}
