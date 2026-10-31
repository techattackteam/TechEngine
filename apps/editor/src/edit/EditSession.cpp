#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Name.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <edit/EditSession.hpp>
#include <edit/EditorSnapshot.hpp>
#include <edit/FieldCollector.hpp>

#include <functional>
#include <utility>

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

    void EditSession::recordSave() {
        m_savedRevision = m_editRevision;
    }

    void EditSession::setSceneFile(SceneFile file) {
        m_sceneFile = std::move(file);
    }

    bool EditSession::getSceneFile(SceneFile& out) const {
        out = m_sceneFile;
        return !out.path.empty();
    }

    void EditSession::record(EditRecord record) {
        m_records.push_back(std::move(record));
    }

    std::span<const EditRecord> EditSession::getRecords() const {
        return m_records;
    }

    void EditSession::buildSnapshot(const Scene& scene, const ComponentRegistry& registry, EditorSnapshot& out) const {
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
        collectComponents(scene, registry, out);
        out.editRevision = m_editRevision;
        out.savedRevision = m_savedRevision;
    }

    void EditSession::collectComponents(const Scene& scene, const ComponentRegistry& registry, EditorSnapshot& out) const {
        out.components.clear();
        out.addable.clear();
        if (!scene.contains(m_selected)) {
            return;
        }

        const ComponentTypeId hierarchyType = componentTypeId<Hierarchy>();
        const ComponentTypeId transformType = componentTypeId<Transform>();
        const ComponentTypeId nameType = componentTypeId<Name>();
        for (const ComponentTypeRecord& record: registry.records()) {
            const void* component = scene.componentRaw(m_selected, record.id);
            if (component != nullptr) {
                if (!record.visitFields || record.id == hierarchyType || record.id == nameType) {
                    continue;
                }

                ComponentRow& row = out.components.emplace_back();
                row.type = record.id;
                row.tag = record.tag;
                row.removable = record.id != transformType;

                FieldCollector collector(scene, row.fields);
                record.visitFields(collector, const_cast<void*>(component));
                continue;
            }

            if (!record.visitFields || record.id == hierarchyType || record.id == transformType || record.id == nameType) {
                continue;
            }

            out.addable.push_back({record.id, record.tag});
        }
    }
}
