#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/scene/components/Name.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <assets/AssetIndex.hpp>
#include <edit/EditRecord.hpp>
#include <edit/EditSession.hpp>
#include <edit/EditorSnapshot.hpp>
#include <edit/FieldCollector.hpp>
#include <edit/FieldSetter.hpp>
#include <edit/FieldValue.hpp>
#include <edit/InspectorCommands.hpp>
#include <edit/MaterialSlotRebuild.hpp>

#include <array>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    // rotation.w's position in Transform's visit, so reordering that visit moves it.
    static constexpr std::uint32_t TRANSFORM_ROTATION_W_FIELD = 3;

    static Quat withEulerAxis(const Quat& rotation, const int axis, const float degrees) {
        Vec3 euler = Transform::toEulerDegrees(rotation);
        euler[axis] = degrees;
        return Transform::fromEulerDegrees(euler);
    }

    SetFieldCommand::SetFieldCommand(EditSession& session, const ComponentRegistry& registry, std::vector<Entity> entities, const ComponentTypeId component, const std::uint32_t field, FieldValue value)
        : m_session(&session), m_registry(&registry), m_entities(std::move(entities)), m_component(component), m_field(field), m_value(std::move(value)) {
    }

    void SetFieldCommand::apply(Scene& scene) {
        const ComponentTypeRecord* record = m_registry->find(m_component);
        if (record == nullptr) {
            TE_LOGGER_WARN("Component type {0} is not registered", m_component.stringId());
            return;
        }

        if (record->editFields == nullptr) {
            TE_LOGGER_WARN("Component type {0} has no editable fields", m_component.stringId());
            return;
        }

        // Name has a visit, so it has an edit walk, but a rename must go through RenameEntityCommand and leave a RenameRecord.
        if (m_component == componentTypeId<Name>()) {
            TE_LOGGER_WARN("SetFieldCommand: Name is changed only by a rename");
            return;
        }

        bool changed = false;
        for (Entity entity: m_entities) {
            if (!entity.valid()) {
                TE_LOGGER_WARN("SetFieldCommand applied with an invalid entity");
                continue;
            }
            if (!scene.contains(entity)) {
                TE_LOGGER_WARN("SetFieldCommand applied with a dead entity");
                continue;
            }
            void* component = scene.componentRaw(entity, m_component);
            if (component == nullptr) {
                TE_LOGGER_WARN("SetFieldCommand applied to entity {0} without component {1}", entity, m_component.stringId());
                continue;
            }

            FieldSetter setter(m_field, m_value);
            record->editFields(setter, component);
            logFieldAfterEdit(scene, *record, entity, component, setter);
            if (!setter.wasApplied() || setter.getOldValue() == m_value) {
                continue;
            }

            FieldEditRecord edit;
            edit.entity = entity;
            edit.component = m_component;
            edit.field = m_field;
            edit.oldValue = setter.getOldValue();
            m_session->record(std::move(edit));
            changed = true;
        }

        if (changed) {
            m_session->recordSceneChange();
        }
    }

    void SetFieldCommand::logFieldAfterEdit(const Scene& scene, const ComponentTypeRecord& record, const Entity entity, void* component, const FieldSetter& setter) const {
        if (!setter.wasApplied()) {
            TE_LOGGER_WARN("Set field {0} of {1} on entity {2} was not applied: no such field, a read-only one, or a value of another type", m_field, record.tag, entity);
            return;
        }

        // Read back from the live component, so the line shows what it holds now, not what was sent.
        std::vector<FieldRow> rows;
        FieldCollector collector(scene, rows);
        record.visitFields(collector, component);
        if (m_field >= rows.size()) {
            return;
        }
        const FieldRow& row = rows[m_field];
        TE_LOGGER_DEBUG("Set {0}.{1} on entity {2}: {3} -> {4}", record.tag, row.name, entity, describeFieldValue(setter.getOldValue()), describeFieldValue(row.value));
    }

    SetAssetFieldCommand::SetAssetFieldCommand(
        EditSession& session, const ComponentRegistry& registry, ResourceCache& resources, const ResourceHandle<SceneBytes>& sceneHandle, const AssetIndex& assets, std::vector<Entity> entities, const ComponentTypeId component, const std::uint32_t field, const ResourceId id)
        : m_session(&session), m_registry(&registry), m_resources(&resources), m_sceneHandle(&sceneHandle), m_assets(&assets), m_entities(std::move(entities)), m_component(component), m_field(field), m_id(id) {
    }

    void SetAssetFieldCommand::apply(Scene& scene) {
        const ComponentTypeRecord* record = m_registry->find(m_component);
        if (record == nullptr || record->visitFields == nullptr || record->editFields == nullptr) {
            TE_LOGGER_WARN("Component type {0} has no editable fields", m_component.stringId());
            return;
        }

        AssetRecord asset;
        const bool indexed = m_assets->find(m_id, asset);
        if (!indexed) {
            TE_LOGGER_WARN("Asset {0:#018x} is not in the asset index, so it is not set", m_id.value());
            return;
        }
        if (asset.side == Side::Server) {
            TE_LOGGER_WARN("Asset {0:#018x} is a server asset, which the editor's client scene cannot load, so it is not set", m_id.value());
            return;
        }

        if (!m_sceneHandle->valid()) {
            TE_LOGGER_WARN("The scene has no cache entry, so asset {0:#018x} is not set", m_id.value());
            return;
        }

        if (!changesAnyField(scene, *record, asset.typeTag)) {
            return;
        }

        ResourceSlotId slot;
        const std::error_code addError = m_resources->addDependency(*m_sceneHandle, m_id, slot);
        if (addError) {
            TE_LOGGER_WARN("Asset {0:#018x} cannot be added to the scene: {1}", m_id.value(), addError.message());
            return;
        }

        const std::error_code loadError = m_resources->runLoadingPhase();
        ResourceState state = ResourceState::Pending;
        const bool found = m_resources->getState(slot, state);
        if (!found || state != ResourceState::Ready) {
            TE_LOGGER_WARN("Asset {0:#018x} did not load, so it is not set: {1}", m_id.value(), loadError ? loadError.message() : std::string("it had already failed"));
            return;
        }

        const FieldValue value = m_id;
        std::vector<Entity> changed;
        for (const Entity entity: m_entities) {
            if (!entity.valid()) {
                TE_LOGGER_WARN("SetAssetFieldCommand applied with an invalid entity");
                continue;
            }
            if (!scene.contains(entity)) {
                TE_LOGGER_WARN("SetAssetFieldCommand applied with a dead entity");
                continue;
            }
            void* component = scene.componentRaw(entity, m_component);
            if (component == nullptr) {
                TE_LOGGER_WARN("SetAssetFieldCommand applied to entity {0} without component {1}", entity, m_component.stringId());
                continue;
            }

            FieldSetter setter(m_field, value, slot);
            record->editFields(setter, component);
            if (!setter.wasApplied() || setter.getOldValue() == value) {
                continue;
            }
            TE_LOGGER_DEBUG("Set asset field {0} of {1} on entity {2}: {3} -> {4}", m_field, record->tag, entity, describeFieldValue(setter.getOldValue()), describeFieldValue(value));

            FieldEditRecord edit;
            edit.entity = entity;
            edit.component = m_component;
            edit.field = m_field;
            edit.oldValue = setter.getOldValue();
            m_session->record(std::move(edit));
            changed.push_back(entity);
        }

        if (!changed.empty()) {
            // A material set rebuilds too, which changes nothing, because its slot names are the mesh's already.
            if (m_component == componentTypeId<MeshRenderer>()) {
                rebuildMaterialSlots(scene, *m_resources, *m_sceneHandle, changed);
            }
            m_session->recordSceneChange();
        }
    }

    bool SetAssetFieldCommand::changesAnyField(Scene& scene, const ComponentTypeRecord& record, const StringId assetType) const {
        std::vector<FieldRow> rows;
        for (const Entity entity: m_entities) {
            if (!entity.valid() || !scene.contains(entity)) {
                continue;
            }
            void* component = scene.componentRaw(entity, m_component);
            if (component == nullptr) {
                continue;
            }

            rows.clear();
            FieldCollector collector(scene, rows);
            record.visitFields(collector, component);
            if (m_field >= rows.size() || rows[m_field].kind != FieldKind::Asset) {
                TE_LOGGER_WARN("Field {0} of {1} is not an asset field, so asset {2:#018x} is not set", m_field, record.tag, m_id.value());
                return false;
            }
            if (rows[m_field].assetType != assetType) {
                TE_LOGGER_WARN("Field {0} of {1} holds another asset type than asset {2:#018x}, so it is not set", m_field, record.tag, m_id.value());
                return false;
            }
            if (rows[m_field].value != FieldValue{m_id}) {
                return true;
            }
        }
        return false;
    }

    SetRotationAxisCommand::SetRotationAxisCommand(EditSession& session, const ComponentRegistry& registry, std::vector<Entity> entities, const Entity active, const Quat activeRotation, const std::uint32_t axis, const float degrees)
        : m_session(&session), m_registry(&registry), m_entities(std::move(entities)), m_active(active), m_activeRotation(activeRotation), m_axis(axis), m_degrees(degrees) {
    }

    void SetRotationAxisCommand::apply(Scene& scene) {
        const ComponentTypeRecord* record = m_registry->find(componentTypeId<Transform>());
        if (record == nullptr || record->editFields == nullptr) {
            TE_LOGGER_WARN("SetRotationAxisCommand: Transform has no edit walk");
            return;
        }

        bool changed = false;
        for (Entity entity: m_entities) {
            if (!entity.valid()) {
                TE_LOGGER_WARN("SetRotationAxisCommand applied with an invalid entity");
                continue;
            }
            if (!scene.contains(entity)) {
                TE_LOGGER_WARN("SetRotationAxisCommand applied with a dead entity");
                continue;
            }

            Transform& transform = scene.getComponent<Transform>(entity);
            const Quat oldRotation = transform.getLocal().rotation;
            Quat rotation = m_activeRotation;
            if (entity != m_active) {
                rotation = withEulerAxis(oldRotation, static_cast<int>(m_axis), m_degrees);
            }
            if (rotation == oldRotation) {
                continue;
            }

            // Not setLocal: it refuses a zero scale, which a field edit allows.
            const bool written = writeRotation(*record, &transform, rotation);
            if (!written) {
                TE_LOGGER_WARN("SetRotationAxisCommand: entity {0} refused its new rotation", entity);
                continue;
            }

            RotationEditRecord record;
            record.entity = entity;
            record.oldRotation = oldRotation;
            m_session->record(std::move(record));
            changed = true;
        }

        if (changed) {
            m_session->recordSceneChange();
        }
    }

    bool SetRotationAxisCommand::writeRotation(const ComponentTypeRecord& record, void* component, const Quat& rotation) const {
        const std::array<float, 4> values = {rotation.w, rotation.x, rotation.y, rotation.z};
        for (std::uint32_t i = 0; i < values.size(); i++) {
            const FieldValue value{values[i]};
            FieldSetter setter(TRANSFORM_ROTATION_W_FIELD + i, value);
            record.editFields(setter, component);
            if (!setter.wasApplied()) {
                return false;
            }
        }
        return true;
    }

    SetWorldAxisCommand::SetWorldAxisCommand(EditSession& session, std::vector<Entity> entities, const Entity active, const Quat activeRotation, const TransformPart part, const std::uint32_t axis, const float value)
        : m_session(&session), m_entities(std::move(entities)), m_active(active), m_activeRotation(activeRotation), m_part(part), m_axis(axis), m_value(value) {
    }

    void SetWorldAxisCommand::apply(Scene& scene) {
        bool changed = false;
        for (Entity entity: m_entities) {
            if (!entity.valid()) {
                TE_LOGGER_WARN("SetWorldAxisCommand applied with an invalid entity");
                continue;
            }
            if (!scene.contains(entity)) {
                TE_LOGGER_WARN("SetWorldAxisCommand applied with a dead entity");
                continue;
            }

            Transform& transform = scene.getComponent<Transform>(entity);
            const TransformValues& world = transform.getWorld();
            const TransformValues target = getTargetWorld(entity, world);
            // setWorld decomposes a matrix, so even an unchanged target would leave float noise in the local values.
            if (target.position == world.position && target.rotation == world.rotation && target.scale == world.scale) {
                continue;
            }

            const TransformValues oldLocal = transform.getLocal();
            const bool written = transform.setWorld(target);
            if (!written) {
                TE_LOGGER_WARN("SetWorldAxisCommand: entity {0} refused its new world transform", entity);
                continue;
            }

            LocalTransformRecord record;
            record.entity = entity;
            record.oldLocal = oldLocal;
            m_session->record(std::move(record));
            changed = true;
        }

        if (changed) {
            m_session->recordSceneChange();
        }
    }

    TransformValues SetWorldAxisCommand::getTargetWorld(const Entity entity, const TransformValues& world) const {
        TransformValues target = world;
        const int axis = static_cast<int>(m_axis);
        switch (m_part) {
            case TransformPart::Position:
                target.position[axis] = m_value;
                break;
            case TransformPart::Scale:
                target.scale[axis] = m_value;
                break;
            case TransformPart::Rotation:
                if (entity == m_active) {
                    target.rotation = m_activeRotation;
                } else {
                    target.rotation = withEulerAxis(world.rotation, axis, m_value);
                }
                break;
        }
        return target;
    }

    AddComponentCommand::AddComponentCommand(EditSession& session, std::vector<Entity> entities, const ComponentTypeId component) : m_session(&session), m_entities(std::move(entities)), m_component(component) {
    }

    void AddComponentCommand::apply(Scene& scene) {
        for (Entity entity: m_entities) {
            if (!entity.valid()) {
                TE_LOGGER_WARN("AddComponentCommand applied with an invalid entity");
                continue;
            }
            if (!scene.contains(entity)) {
                TE_LOGGER_WARN("AddComponentCommand applied with a dead entity");
                continue;
            }
            void* component = scene.componentRaw(entity, m_component);
            if (component == nullptr) {
                const bool added = scene.addComponentRaw(entity, m_component);
                if (!added) {
                    TE_LOGGER_WARN("AddComponentCommand: Failed to add component {0} to entity {1}", m_component.stringId(), entity);
                    continue;
                }

                ComponentAddedRecord record;
                record.entity = entity;
                record.component = m_component;
                m_session->record(std::move(record));
                m_session->recordSceneChange();
            }
        }
    }

    RemoveComponentCommand::RemoveComponentCommand(EditSession& session, const ComponentRegistry& registry, std::vector<Entity> entities, const ComponentTypeId component) : m_session(&session), m_registry(&registry), m_entities(std::move(entities)), m_component(component) {
    }

    void RemoveComponentCommand::apply(Scene& scene) {
        for (Entity entity: m_entities) {

            ComponentRemovedRecord record;
            record.entity = entity;
            record.component = m_component;
            // The payload is the old value, so it has to be written before the component is gone.
            const ComponentTypeRecord* typeRecord = m_registry->find(m_component);
            const void* component = scene.componentRaw(entity, m_component);
            if (typeRecord != nullptr && typeRecord->writePayload != nullptr && component != nullptr) {
                Writer writer{record.payload};
                typeRecord->writePayload(writer, component);
            }

            const bool removed = scene.removeComponentRaw(entity, m_component);
            if (!removed) {
                TE_LOGGER_WARN("RemoveComponentCommand: Failed to remove component {0} from entity {1}", m_component.stringId(), entity);
                continue;
            }

            m_session->record(std::move(record));
            m_session->recordSceneChange();
        }
    }
}
