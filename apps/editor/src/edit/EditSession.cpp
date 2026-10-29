#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Name.hpp>

#include <edit/EditSession.hpp>
#include <edit/EditorSnapshot.hpp>

#include <functional>

namespace TechEngine {
    void EditSession::setSelected(const Entity entity) {
        m_selected = entity;
    }

    Entity EditSession::getSelected() const {
        return m_selected;
    }

    void EditSession::recordSceneChange() {
        m_editRevision++;
    }

    void EditSession::buildSnapshot(const Scene& scene, EditorSnapshot& out) const {
        out.hierarchy.clear();

        const std::function<void(Entity, std::uint32_t)> appendSubtree = [&](const Entity entity, const std::uint32_t depth) {
            HierarchyRow& row = out.hierarchy.emplace_back();
            row.entity = entity;
            row.depth = depth;
            row.name = scene.getComponent<Name>(entity).value;

            const std::vector<Entity> children = scene.getChildren(entity);
            row.childCount = children.size();
            for (const Entity child: children) {
                appendSubtree(child, depth + 1);
            }
        };

        for (const Entity root: scene.getRoots()) {
            appendSubtree(root, 0);
        }

        out.selected = m_selected;
        out.editRevision = m_editRevision;
        out.savedRevision = m_savedRevision;
    }
}
