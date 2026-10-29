#pragma once

#include <TechEngine/core/scene/Entity.hpp>

#include <cstddef>
#include <memory>
#include <string>

namespace TechEngine {
    class App;
    class Command;
    class EditSession;

    class EditorCommands {
    private:
        App* m_app = nullptr;
        // Only handed to commands; reading or writing through it on main races the simulation thread.
        EditSession* m_session = nullptr;

    public:
        EditorCommands(App& app, EditSession& session);

        void create(Entity parent);

        void destroy(Entity entity);

        void reparent(Entity child, Entity parent, std::size_t position);

        void rename(Entity entity, std::string name);

        void select(Entity entity);

    private:
        void submit(std::unique_ptr<Command> command);
    };
}
