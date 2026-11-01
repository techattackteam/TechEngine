#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Name.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <edit/EditSession.hpp>
#include <edit/EditorSnapshot.hpp>
#include <edit/FieldCollector.hpp>

#include <algorithm>
#include <functional>
#include <utility>

namespace TechEngine {
    void EditSession::setSelection(std::vector<Entity> selection, const Entity active, const std::uint64_t sequence) {
        if (sequence < m_selectionSequence) {
            return;
        }
        m_selection = std::move(selection);
        m_active = active;
        m_selectionSequence = sequence;
    }

    std::span<const Entity> EditSession::getSelection() const {
        return m_selection;
    }

    Entity EditSession::getActive() const {
        return m_active;
    }

    std::uint64_t EditSession::getSelectionSequence() const {
        return m_selectionSequence;
    }

    void EditSession::removeDeadFromSelection(const Scene& scene) {
        std::erase_if(m_selection, [&](const Entity entity) {
            return !scene.contains(entity);
        });
        if (!scene.contains(m_active)) {
            m_active = {};
        }
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
        if (m_sceneFile.path.empty()) {
            return false;
        }
        out = m_sceneFile;
        return true;
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

        out.selection = m_selection;
        out.active = m_active;
        out.selectionSequence = m_selectionSequence;
        collectComponents(scene, registry, out);
        out.editRevision = m_editRevision;
        out.savedRevision = m_savedRevision;
    }

    void EditSession::collectComponents(const Scene& scene, const ComponentRegistry& registry, EditorSnapshot& out) const {
        out.components.clear();
        out.addable.clear();
        if (!scene.contains(m_active)) {
            return;
        }

        const ComponentTypeId hierarchyType = componentTypeId<Hierarchy>();
        const ComponentTypeId transformType = componentTypeId<Transform>();
        const ComponentTypeId nameType = componentTypeId<Name>();
        for (const ComponentTypeRecord& record: registry.records()) {
            const void* component = scene.componentRaw(m_active, record.id);
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
