#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Name.hpp>

#include <edit/EditSession.hpp>
#include <edit/HierarchyCommands.hpp>

#include <utility>

namespace TechEngine {
    CreateEntityCommand::CreateEntityCommand(EditSession& session, const Entity parent) : m_session(&session), m_parent(parent) {
    }

    void CreateEntityCommand::apply(Scene& scene) {
        Entity newEntity = scene.createEntity();

        if (m_parent.valid()) {
            const bool parented = scene.setParent(newEntity, m_parent, 0, ReparentMode::PreserveWorld);
            if (!parented) {
                TE_LOGGER_ERROR("Failed to parent new entity {0} under {1}", newEntity, m_parent);
                scene.destroyEntity(newEntity);
                return;
            }
        }

        m_session->recordSceneChange();
    }

    DestroyEntityCommand::DestroyEntityCommand(EditSession& session, const Entity entity) : m_session(&session), m_entity(entity) {
    }

    void DestroyEntityCommand::apply(Scene& scene) {
        if (!m_entity.valid()) {
            TE_LOGGER_WARN("DestroyEntityCommand applied with an invalid entity");
            return;
        }

        if (!scene.contains(m_entity)) {
            TE_LOGGER_WARN("DestroyEntityCommand applied with a dead entity");
            return;
        }

        const bool destroyed = scene.destroyEntity(m_entity);
        if (!destroyed) {
            TE_LOGGER_WARN("DestroyEntityCommand failed to destroy entity {0}", m_entity);
            return;
        }

        if (!scene.contains(m_session->getSelected())) {
            m_session->setSelected(Entity{});
        }
        m_session->recordSceneChange();
    }

    ReparentEntityCommand::ReparentEntityCommand(EditSession& session, const Entity child, const Entity parent, const std::size_t position) : m_session(&session), m_child(child), m_parent(parent), m_position(position) {
    }

    void ReparentEntityCommand::apply(Scene& scene) {
        if (!m_child.valid()) {
            TE_LOGGER_WARN("ReparentEntityCommand applied with an invalid child");
            return;
        }
        if (!scene.contains(m_child)) {
            TE_LOGGER_WARN("ReparentEntityCommand applied with a dead child");
            return;
        }
        if (!m_parent.valid()) {
            scene.unparent(m_child);
        } else {
            const bool parented = scene.setParent(m_child, m_parent, m_position, ReparentMode::PreserveWorld);
            if (!parented) {
                TE_LOGGER_ERROR("Failed to reparent entity {0} under {1} at position {2}", m_child, m_parent, m_position);
                return;
            }
        }
        m_session->recordSceneChange();
    }

    RenameEntityCommand::RenameEntityCommand(EditSession& session, const Entity entity, std::string name) : m_session(&session), m_entity(entity), m_name(std::move(name)) {
    }

    void RenameEntityCommand::apply(Scene& scene) {
        if (!scene.contains(m_entity)) {
            TE_LOGGER_WARN("RenameEntityCommand applied with a dead or invalid entity");
            return;
        }

        Name& name = scene.getComponent<Name>(m_entity);
        if (name.value == m_name) {
            return;
        }
        name.value = std::move(m_name);
        m_session->recordSceneChange();
    }

    SelectEntityCommand::SelectEntityCommand(EditSession& session, const Entity entity) : m_session(&session), m_entity(entity) {
    }

    void SelectEntityCommand::apply(Scene& scene) {
        if (!m_entity.valid()) {
            m_session->setSelected(Entity{});
            return;
        }

        if (!scene.contains(m_entity)) {
            m_session->setSelected(m_session->getSelected());
            return;
        }

        m_session->setSelected(m_entity);
    }
}
