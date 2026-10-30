#pragma once

#include <TechEngine/app/Command.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <edit/FieldValue.hpp>

#include <cstdint>

namespace TechEngine {
    class ComponentRegistry;
    class EditSession;
    class FieldSetter;
    struct ComponentTypeRecord;

    class SetFieldCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        const ComponentRegistry* m_registry = nullptr;
        Entity m_entity;
        ComponentTypeId m_component;
        std::uint32_t m_field = 0;
        FieldValue m_value;

    public:
        SetFieldCommand(EditSession& session, const ComponentRegistry& registry, Entity entity, ComponentTypeId component, std::uint32_t field, FieldValue value);

        void apply(Scene& scene) override;

    private:
        void logFieldAfterEdit(const Scene& scene, const ComponentTypeRecord& record, void* component, const FieldSetter& setter) const;
    };

    class AddComponentCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        Entity m_entity;
        ComponentTypeId m_component;

    public:
        AddComponentCommand(EditSession& session, Entity entity, ComponentTypeId component);

        void apply(Scene& scene) override;
    };

    class RemoveComponentCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        const ComponentRegistry* m_registry = nullptr;
        Entity m_entity;
        ComponentTypeId m_component;

    public:
        RemoveComponentCommand(EditSession& session, const ComponentRegistry& registry, Entity entity, ComponentTypeId component);

        void apply(Scene& scene) override;
    };
}
