#pragma once

#include <TechEngine/app/Command.hpp>
#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <edit/FieldValue.hpp>

#include <cstdint>
#include <vector>

namespace TechEngine {
    class ComponentRegistry;
    class EditSession;
    class FieldSetter;
    struct ComponentTypeRecord;

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
