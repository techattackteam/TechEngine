#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Name.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <edit/EditRecord.hpp>
#include <edit/EditSession.hpp>
#include <edit/EditorSnapshot.hpp>
#include <edit/FieldCollector.hpp>
#include <edit/FieldSetter.hpp>
#include <edit/InspectorCommands.hpp>

#include <array>
#include <utility>
#include <vector>

namespace TechEngine {
    // rotation.w's position in Transform's visit, so reordering that visit moves it.
    static constexpr std::uint32_t TRANSFORM_ROTATION_W_FIELD = 3;

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
                Vec3 euler = Transform::toEulerDegrees(oldRotation);
                euler[static_cast<int>(m_axis)] = m_degrees;
                rotation = Transform::fromEulerDegrees(euler);
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
