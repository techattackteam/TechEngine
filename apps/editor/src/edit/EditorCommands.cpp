#include <TechEngine/app/App.hpp>
#include <TechEngine/app/Command.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>

#include <edit/EditorCommands.hpp>
#include <edit/HierarchyCommands.hpp>

#include <system_error>
#include <utility>

namespace TechEngine {
    EditorCommands::EditorCommands(App& app, EditSession& session) : m_app(&app), m_session(&session) {
    }

    void EditorCommands::create(const Entity parent) {
        submit(std::make_unique<CreateEntityCommand>(*m_session, parent));
    }

    void EditorCommands::destroy(const Entity entity) {
        submit(std::make_unique<DestroyEntityCommand>(*m_session, entity));
    }

    void EditorCommands::reparent(const Entity child, const Entity parent, const std::size_t position) {
        submit(std::make_unique<ReparentEntityCommand>(*m_session, child, parent, position));
    }

    void EditorCommands::rename(const Entity entity, std::string name) {
        submit(std::make_unique<RenameEntityCommand>(*m_session, entity, std::move(name)));
    }

    void EditorCommands::select(const Entity entity) {
        submit(std::make_unique<SelectEntityCommand>(*m_session, entity));
    }

    void EditorCommands::submit(std::unique_ptr<Command> command) {
        const std::error_code submitError = m_app->submitCommand(std::move(command));
        if (submitError) {
            TE_LOGGER_WARN("The simulation refused an edit: {0}", submitError.message());
        }
    }
}
