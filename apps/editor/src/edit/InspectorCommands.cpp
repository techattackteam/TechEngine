#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <edit/EditRecord.hpp>
#include <edit/EditSession.hpp>
#include <edit/EditorSnapshot.hpp>
#include <edit/FieldCollector.hpp>
#include <edit/FieldSetter.hpp>
#include <edit/InspectorCommands.hpp>

#include <utility>
#include <vector>

namespace TechEngine {
    SetFieldCommand::SetFieldCommand(EditSession& session, const ComponentRegistry& registry, const Entity entity, const ComponentTypeId component, const std::uint32_t field, FieldValue value)
        : m_session(&session), m_registry(&registry), m_entity(entity), m_component(component), m_field(field), m_value(std::move(value)) {
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

        void* component = scene.componentRaw(m_entity, m_component);
        if (component == nullptr) {
            TE_LOGGER_WARN("SetFieldCommand: Component not found in scene");
            return;
        }

        FieldSetter setter(m_field, m_value);
        record->editFields(setter, component);
        logFieldAfterEdit(scene, *record, component, setter);
        if (!setter.wasApplied() || setter.getOldValue() == m_value) {
            return;
        }

        FieldEditRecord edit;
        edit.entity = m_entity;
        edit.component = m_component;
        edit.field = m_field;
        edit.oldValue = setter.getOldValue();
        m_session->record(std::move(edit));
        m_session->recordSceneChange();
    }

    void SetFieldCommand::logFieldAfterEdit(const Scene& scene, const ComponentTypeRecord& record, void* component, const FieldSetter& setter) const {
        if (!setter.wasApplied()) {
            TE_LOGGER_WARN("Set field {0} of {1} on entity {2} was not applied: no such field, a read-only one, or a value of another type", m_field, record.tag, m_entity);
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
        TE_LOGGER_INFO("Set {0}.{1} on entity {2}: {3} -> {4}", record.tag, row.name, m_entity, describeFieldValue(setter.getOldValue()), describeFieldValue(row.value));
    }

    AddComponentCommand::AddComponentCommand(EditSession& session, const Entity entity, const ComponentTypeId component) : m_session(&session), m_entity(entity), m_component(component) {
    }

    void AddComponentCommand::apply(Scene& scene) {
        const bool added = scene.addComponentRaw(m_entity, m_component);
        if (!added) {
            TE_LOGGER_WARN("AddComponentCommand: Failed to add component {0} to entity {1}", m_component.stringId(), m_entity);
            return;
        }

        ComponentAddedRecord record;
        record.entity = m_entity;
        record.component = m_component;
        m_session->record(std::move(record));
        m_session->recordSceneChange();
    }

    RemoveComponentCommand::RemoveComponentCommand(EditSession& session, const ComponentRegistry& registry, const Entity entity, const ComponentTypeId component) : m_session(&session), m_registry(&registry), m_entity(entity), m_component(component) {
    }

    void RemoveComponentCommand::apply(Scene& scene) {
        ComponentRemovedRecord record;
        record.entity = m_entity;
        record.component = m_component;
        // The payload is the old value, so it has to be written before the component is gone.
        const ComponentTypeRecord* typeRecord = m_registry->find(m_component);
        const void* component = scene.componentRaw(m_entity, m_component);
        if (typeRecord != nullptr && typeRecord->writePayload != nullptr && component != nullptr) {
            Writer writer{record.payload};
            typeRecord->writePayload(writer, component);
        }

        const bool removed = scene.removeComponentRaw(m_entity, m_component);
        if (!removed) {
            TE_LOGGER_WARN("RemoveComponentCommand: Failed to remove component {0} from entity {1}", m_component.stringId(), m_entity);
            return;
        }

        m_session->record(std::move(record));
        m_session->recordSceneChange();
    }
}
