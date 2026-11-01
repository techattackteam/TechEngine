#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Name.hpp>

#include <edit/EditSession.hpp>
#include <edit/HierarchyCommands.hpp>

#include <algorithm>
#include <utility>

namespace TechEngine {
    CreateEntityCommand::CreateEntityCommand(EditSession& session, const Entity parent) : m_session(&session), m_parent(parent) {
    }

    void CreateEntityCommand::apply(Scene& scene) {
        Entity newEntity = scene.createEntity();

        if (m_parent.valid()) {
            const bool parented = scene.setParent(newEntity, m_parent, 0, ReparentMode::PreserveWorld);
            if (!parented) {
                TE_LOGGER_WARN("Failed to parent new entity {0} under {1}", newEntity, m_parent);
                scene.destroyEntity(newEntity);
                return;
            }
        }

        m_session->recordSceneChange();
    }

    DestroyEntityCommand::DestroyEntityCommand(EditSession& session, std::vector<Entity> entities) : m_session(&session), m_entities(std::move(entities)) {
    }

    void DestroyEntityCommand::apply(Scene& scene) {
        bool anyDestroyed = false;
        for (const Entity entity: m_entities) {
            if (!entity.valid()) {
                TE_LOGGER_WARN("DestroyEntityCommand applied with an invalid entity");
                continue;
            }

            if (!scene.contains(entity)) {
                TE_LOGGER_WARN("DestroyEntityCommand applied with a dead entity");
                continue;
            }

            const bool destroyed = scene.destroyEntity(entity);
            if (!destroyed) {
                TE_LOGGER_WARN("DestroyEntityCommand failed to destroy entity {0}", entity);
                continue;
            }
            anyDestroyed = true;
        }

        if (!anyDestroyed) {
            return;
        }
        m_session->removeDeadFromSelection(scene);
        m_session->recordSceneChange();
    }

    ReparentEntityCommand::ReparentEntityCommand(EditSession& session, std::vector<Entity> children, const Entity parent, const std::size_t position) : m_session(&session), m_children(std::move(children)), m_parent(parent), m_position(position) {
    }

    void ReparentEntityCommand::apply(Scene& scene) {
        for (const Entity child: m_children) {
            if (!child.valid()) {
                TE_LOGGER_WARN("ReparentEntityCommand applied with an invalid child");
                return;
            }
            if (!scene.contains(child)) {
                TE_LOGGER_WARN("ReparentEntityCommand applied with a dead child");
                return;
            }
        }

        bool anyMoved = false;
        if (!m_parent.valid()) {
            for (const Entity child: m_children) {
                if (!scene.getParent(child).valid()) {
                    continue;
                }
                const bool unparented = scene.unparent(child, ReparentMode::PreserveWorld);
                if (!unparented) {
                    TE_LOGGER_WARN("Failed to make entity {0} a root", child);
                    continue;
                }
                anyMoved = true;
            }
            if (anyMoved) {
                m_session->recordSceneChange();
            }
            return;
        }

        if (!scene.contains(m_parent)) {
            TE_LOGGER_WARN("ReparentEntityCommand applied with a dead parent {0}", m_parent);
            return;
        }
        if (isInMovedSubtree(scene, m_parent)) {
            TE_LOGGER_WARN("ReparentEntityCommand refused to move entities into their own subtree under {0}", m_parent);
            return;
        }

        Entity previous = getInsertAnchor(scene);
        for (const Entity child: m_children) {
            const std::size_t position = getPositionAfter(scene, child, previous);
            const bool parented = scene.setParent(child, m_parent, position, ReparentMode::PreserveWorld);
            if (!parented) {
                TE_LOGGER_WARN("Failed to reparent entity {0} under {1} at position {2}", child, m_parent, position);
                continue;
            }
            previous = child;
            anyMoved = true;
        }
        if (anyMoved) {
            m_session->recordSceneChange();
        }
    }

    bool ReparentEntityCommand::isMoved(const Entity entity) const {
        return std::ranges::find(m_children, entity) != m_children.end();
    }

    bool ReparentEntityCommand::isInMovedSubtree(const Scene& scene, const Entity entity) const {
        for (Entity ancestor = entity; ancestor.valid(); ancestor = scene.getParent(ancestor)) {
            if (isMoved(ancestor)) {
                return true;
            }
        }
        return false;
    }

    Entity ReparentEntityCommand::getInsertAnchor(const Scene& scene) const {
        Entity anchor;
        std::size_t kept = 0;
        for (const Entity sibling: scene.getChildren(m_parent)) {
            if (kept == m_position) {
                break;
            }
            if (!isMoved(sibling)) {
                anchor = sibling;
                kept++;
            }
        }
        return anchor;
    }

    std::size_t ReparentEntityCommand::getPositionAfter(const Scene& scene, const Entity child, const Entity anchor) const {
        if (!anchor.valid()) {
            return 0;
        }
        std::size_t position = 0;
        for (const Entity sibling: scene.getChildren(m_parent)) {
            if (sibling == child) {
                continue;
            }
            position++;
            if (sibling == anchor) {
                break;
            }
        }
        return position;
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

        RenameRecord record;
        record.entity = m_entity;
        record.oldName = std::move(name.value);
        name.value = std::move(m_name);
        m_session->record(std::move(record));
        m_session->recordSceneChange();
    }

    SelectEntityCommand::SelectEntityCommand(EditSession& session, std::vector<Entity> selection, const Entity active, const std::uint64_t sequence) : m_session(&session), m_selection(std::move(selection)), m_active(active), m_sequence(sequence) {
    }

    void SelectEntityCommand::apply(Scene& scene) {
        m_session->setSelection(std::move(m_selection), m_active, m_sequence);
        m_session->removeDeadFromSelection(scene);
    }
}
