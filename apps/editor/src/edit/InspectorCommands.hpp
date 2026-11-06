#pragma once

#include <TechEngine/app/Command.hpp>
#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <edit/FieldValue.hpp>

#include <cstdint>
#include <vector>

namespace TechEngine {
    class AssetIndex;
    class ComponentRegistry;
    class EditSession;
    class FieldSetter;
    class ResourceCache;
    struct ComponentTypeRecord;
    struct SceneBytes;

    class SetFieldCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        const ComponentRegistry* m_registry = nullptr;
        std::vector<Entity> m_entities;
        ComponentTypeId m_component;
        std::uint32_t m_field = 0;
        FieldValue m_value;

    public:
        SetFieldCommand(EditSession& session, const ComponentRegistry& registry, std::vector<Entity> entities, ComponentTypeId component, std::uint32_t field, FieldValue value);

        void apply(Scene& scene) override;

    private:
        void logFieldAfterEdit(const Scene& scene, const ComponentTypeRecord& record, Entity entity, void* component, const FieldSetter& setter) const;
    };

    // Refused for an id the asset index does not hold, a server asset, an id of another asset type than the field's, and
    // a scene with no cache entry. The asset loads before any field changes, so a failed load leaves every field as it was; the
    // failed entry stays a dependency of the scene until the scene is released.
    class SetAssetFieldCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        const ComponentRegistry* m_registry = nullptr;
        ResourceCache* m_resources = nullptr;
        // Read at apply, because the startup load sets it after the sender is built.
        const ResourceHandle<SceneBytes>* m_sceneHandle = nullptr;
        const AssetIndex* m_assets = nullptr;
        std::vector<Entity> m_entities;
        ComponentTypeId m_component;
        std::uint32_t m_field = 0;
        ResourceId m_id;

    public:
        SetAssetFieldCommand(EditSession& session, const ComponentRegistry& registry, ResourceCache& resources, const ResourceHandle<SceneBytes>& sceneHandle, const AssetIndex& assets, std::vector<Entity> entities, ComponentTypeId component, std::uint32_t field, ResourceId id);

        void apply(Scene& scene) override;

    private:
        // False, with a warning, when the field is not an asset field of assetType; false, silently, when every
        // entity that holds the component already holds the id, so the scene gains no dependency it does not need.
        bool changesAnyField(Scene& scene, const ComponentTypeRecord& record, StringId assetType) const;
    };

    class SetRotationAxisCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        const ComponentRegistry* m_registry = nullptr;
        std::vector<Entity> m_entities;
        Entity m_active;
        Quat m_activeRotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
        std::uint32_t m_axis = 0;
        float m_degrees = 0.0f;

    public:
        // The active entity takes activeRotation as it is, so it matches the panel's Euler cache exactly; every other
        // entity has its own rotation decomposed, and only axis replaced.
        SetRotationAxisCommand(EditSession& session, const ComponentRegistry& registry, std::vector<Entity> entities, Entity active, Quat activeRotation, std::uint32_t axis, float degrees);

        void apply(Scene& scene) override;

    private:
        bool writeRotation(const ComponentTypeRecord& record, void* component, const Quat& rotation) const;
    };

    enum class TransformPart : std::uint8_t { Position, Rotation, Scale };

    class SetWorldAxisCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        std::vector<Entity> m_entities;
        Entity m_active;
        Quat m_activeRotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
        TransformPart m_part = TransformPart::Position;
        std::uint32_t m_axis = 0;
        float m_value = 0.0f;

    public:
        // A rotation value is in degrees. As in SetRotationAxisCommand, the active entity takes activeRotation as it is,
        // and every other entity has its own world rotation decomposed, with only axis replaced.
        SetWorldAxisCommand(EditSession& session, std::vector<Entity> entities, Entity active, Quat activeRotation, TransformPart part, std::uint32_t axis, float value);

        void apply(Scene& scene) override;

    private:
        TransformValues getTargetWorld(Entity entity, const TransformValues& world) const;
    };

    class AddComponentCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        std::vector<Entity> m_entities;
        ComponentTypeId m_component;

    public:
        AddComponentCommand(EditSession& session, std::vector<Entity> entities, ComponentTypeId component);

        void apply(Scene& scene) override;
    };

    class RemoveComponentCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        const ComponentRegistry* m_registry = nullptr;
        std::vector<Entity> m_entities;
        ComponentTypeId m_component;

    public:
        RemoveComponentCommand(EditSession& session, const ComponentRegistry& registry, std::vector<Entity> entities, ComponentTypeId component);

        void apply(Scene& scene) override;
    };
}
