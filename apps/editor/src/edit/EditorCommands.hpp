#pragma once

#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <edit/FieldValue.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace TechEngine {
    class App;
    class Command;
    class ComponentRegistry;
    class EditSession;

    class EditorCommands {
    private:
        App* m_app = nullptr;
        // Only handed to commands; reading or writing through it on main races the simulation thread.
        EditSession* m_session = nullptr;
        const ComponentRegistry* m_registry = nullptr;

    public:
        EditorCommands(App& app, EditSession& session, const ComponentRegistry& registry);

        void create(Entity parent);

        void destroy(Entity entity);

        void reparent(Entity child, Entity parent, std::size_t position);

        void rename(Entity entity, std::string name);

        void select(Entity entity);

        void setField(Entity entity, ComponentTypeId component, std::uint32_t field, FieldValue value);

        void addComponent(Entity entity, ComponentTypeId component);

        void removeComponent(Entity entity, ComponentTypeId component);

    private:
        void submit(std::unique_ptr<Command> command);
    };
}
