#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Name.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <edit/EditSession.hpp>
#include <edit/EditorSnapshot.hpp>
#include <edit/FieldCollector.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <utility>

namespace TechEngine {
    // Must match the inspector's EULER_DISPLAY_EPSILON: half of the 0.01 that "%.2f" shows, so angles that print the same are never mixed.
    static constexpr float EULER_MIXED_EPSILON = 0.005f;

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

    bool EditSession::hasUnsavedEdits() const {
        return m_editRevision != m_savedRevision;
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
        out.rotationAxesMixed = {};
        out.world = {};
        out.addable.clear();
        // contains(), not valid(): a selection whose entity died would otherwise offer every type to add.
        if (!scene.contains(m_active)) {
            return;
        }

        const ComponentTypeId hierarchyType = componentTypeId<Hierarchy>();
        const ComponentTypeId transformType = componentTypeId<Transform>();
        const ComponentTypeId nameType = componentTypeId<Name>();
        for (const ComponentTypeRecord& record: registry.records()) {
            if (!record.visitFields || record.id == hierarchyType || record.id == nameType) {
                continue;
            }

            bool allHave = true;
            for (const Entity entity: m_selection) {
                if (scene.contains(entity) && scene.componentRaw(entity, record.id) == nullptr) {
                    allHave = false;
                    break;
                }
            }

            const void* activeComponent = scene.componentRaw(m_active, record.id);
            if (!allHave || activeComponent == nullptr) {
                if (record.id != transformType) {
                    out.addable.push_back({record.id, record.tag});
                }
                continue;
            }

            ComponentRow& row = out.components.emplace_back();
            row.type = record.id;
            row.tag = record.tag;
            row.removable = record.id != transformType;

            FieldCollector collector(scene, row.fields);
            record.visitFields(collector, const_cast<void*>(activeComponent));
            markMixedFields(scene, record, row);
            if (record.id == transformType) {
                markMixedRotationAxes(scene, false, out.rotationAxesMixed);
                collectWorldTransform(scene, out);
            }
        }
    }

    void EditSession::markMixedFields(const Scene& scene, const ComponentTypeRecord& record, ComponentRow& row) const {
        std::vector<FieldRow> otherFields;
        for (const Entity entity: m_selection) {
            if (entity == m_active || !scene.contains(entity)) {
                continue;
            }

            otherFields.clear();
            FieldCollector collector(scene, otherFields);
            record.visitFields(collector, const_cast<void*>(scene.componentRaw(entity, record.id)));
            for (std::size_t i = 0; i < row.fields.size() && i < otherFields.size(); i++) {
                if (otherFields[i].value != row.fields[i].value) {
                    row.fields[i].mixed = true;
                }
            }
        }
    }

    void EditSession::collectWorldTransform(const Scene& scene, EditorSnapshot& out) const {
        const TransformValues& active = scene.getComponent<Transform>(m_active).getWorld();
        out.world.position = active.position;
        out.world.rotation = active.rotation;
        out.world.scale = active.scale;
        for (const Entity entity: m_selection) {
            if (entity == m_active || !scene.contains(entity)) {
                continue;
            }

            const TransformValues& world = scene.getComponent<Transform>(entity).getWorld();
            for (int axis = 0; axis < 3; axis++) {
                const std::size_t index = static_cast<std::size_t>(axis);
                if (world.position[axis] != active.position[axis]) {
                    out.world.positionMixed[index] = true;
                }
                if (world.scale[axis] != active.scale[axis]) {
                    out.world.scaleMixed[index] = true;
                }
            }
        }
        markMixedRotationAxes(scene, true, out.world.rotationAxesMixed);
    }

    void EditSession::markMixedRotationAxes(const Scene& scene, const bool world, std::array<bool, 3>& out) const {
        const Transform& activeTransform = scene.getComponent<Transform>(m_active);
        const Quat& activeRotation = world ? activeTransform.getWorld().rotation : activeTransform.getLocal().rotation;
        const Vec3 activeAngles = Transform::toEulerDegrees(activeRotation);
        for (const Entity entity: m_selection) {
            if (entity == m_active || !scene.contains(entity)) {
                continue;
            }

            const Transform& transform = scene.getComponent<Transform>(entity);
            const Quat& rotation = world ? transform.getWorld().rotation : transform.getLocal().rotation;
            const Vec3 angles = Transform::toEulerDegrees(rotation);
            for (int axis = 0; axis < 3; axis++) {
                const float difference = std::fmod(std::abs(angles[axis] - activeAngles[axis]), 360.0f);
                if (std::min(difference, 360.0f - difference) >= EULER_MIXED_EPSILON) {
                    out[static_cast<std::size_t>(axis)] = true;
                }
            }
        }
    }
}
