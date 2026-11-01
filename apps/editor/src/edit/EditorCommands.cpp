#include <TechEngine/app/App.hpp>
#include <TechEngine/app/Command.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>

#include <edit/EditorCommands.hpp>
#include <edit/HierarchyCommands.hpp>
#include <edit/InspectorCommands.hpp>
#include <edit/SaveSceneCommand.hpp>

#include <system_error>
#include <utility>

namespace TechEngine {
    EditorCommands::EditorCommands(App& app, EditSession& session, const ComponentRegistry& registry, FileAccess& files) : m_app(&app), m_session(&session), m_registry(&registry), m_files(&files) {
    }

    void EditorCommands::create(const Entity parent) {
        submit(std::make_unique<CreateEntityCommand>(*m_session, parent));
    }

    void EditorCommands::destroy(std::vector<Entity> entities) {
        submit(std::make_unique<DestroyEntityCommand>(*m_session, std::move(entities)));
    }

    void EditorCommands::reparent(std::vector<Entity> children, const Entity parent, const std::size_t position) {
        submit(std::make_unique<ReparentEntityCommand>(*m_session, std::move(children), parent, position));
    }

    void EditorCommands::rename(const Entity entity, std::string name) {
        submit(std::make_unique<RenameEntityCommand>(*m_session, entity, std::move(name)));
    }

    void EditorCommands::select(std::vector<Entity> selection, const Entity active, const std::uint64_t sequence) {
        submit(std::make_unique<SelectEntityCommand>(*m_session, std::move(selection), active, sequence));
    }

    void EditorCommands::setField(const Entity entity, const ComponentTypeId component, const std::uint32_t field, FieldValue value) {
        submit(std::make_unique<SetFieldCommand>(*m_session, *m_registry, entity, component, field, std::move(value)));
    }

    void EditorCommands::addComponent(const Entity entity, const ComponentTypeId component) {
        submit(std::make_unique<AddComponentCommand>(*m_session, entity, component));
    }

    void EditorCommands::removeComponent(const Entity entity, const ComponentTypeId component) {
        submit(std::make_unique<RemoveComponentCommand>(*m_session, *m_registry, entity, component));
    }

    void EditorCommands::save() {
        submit(std::make_unique<SaveSceneCommand>(*m_session, *m_files, *m_app, AfterSave::KeepOpen));
    }

    void EditorCommands::saveAndClose() {
        submit(std::make_unique<SaveSceneCommand>(*m_session, *m_files, *m_app, AfterSave::Close));
    }

    void EditorCommands::submit(std::unique_ptr<Command> command) {
        const std::error_code submitError = m_app->submitCommand(std::move(command));
        if (submitError) {
            TE_LOGGER_WARN("The simulation refused an edit: {0}", submitError.message());
        }
    }
}
