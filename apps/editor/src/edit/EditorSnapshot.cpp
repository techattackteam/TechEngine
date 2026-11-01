#include <edit/EditorSnapshot.hpp>

#include <algorithm>

namespace TechEngine {
    bool EditorSnapshot::hasUnsavedEdits() const {
        if (editRevision != savedRevision) {
            return true;
        }
        return false;
    }

    bool EditorSnapshot::isInSubtree(const Entity entity, const Entity root) const {
        if (!root.valid()) {
            return false;
        }

        std::size_t rootIndex = hierarchy.size();
        for (std::size_t i = 0; i < hierarchy.size(); i++) {
            if (hierarchy[i].entity == root) {
                rootIndex = i;
                break;
            }
        }

        if (rootIndex == hierarchy.size()) {
            return false;
        }

        if (entity == root) {
            return true;
        }

        const std::uint32_t rootDepth = hierarchy[rootIndex].depth;
        for (std::size_t i = rootIndex + 1; i < hierarchy.size(); i++) {
            if (hierarchy[i].depth <= rootDepth) {
                break;
            }
            if (hierarchy[i].entity == entity) {
                return true;
            }
        }

        return false;
    }

    bool EditorSnapshot::findRow(const Entity entity, std::size_t& out) const {
        for (std::size_t index = 0; index < hierarchy.size(); index++) {
            if (hierarchy[index].entity == entity) {
                out = index;
                return true;
            }
        }
        return false;
    }

    Entity EditorSnapshot::getParentOfRow(const std::size_t row) const {
        for (std::size_t previous = row; previous > 0; previous--) {
            if (hierarchy[previous - 1].depth + 1 == hierarchy[row].depth) {
                return hierarchy[previous - 1].entity;
            }
        }
        return {};
    }

    std::size_t EditorSnapshot::getSiblingIndexOfRow(const std::size_t row) const {
        const std::uint32_t depth = hierarchy[row].depth;
        std::size_t siblingsBefore = 0;
        for (std::size_t previous = row; previous > 0 && hierarchy[previous - 1].depth >= depth; previous--) {
            if (hierarchy[previous - 1].depth == depth) {
                siblingsBefore++;
            }
        }
        return siblingsBefore;
    }

    bool EditorSnapshot::resolveDrop(const Entity dragged, const Entity parent, const std::size_t gap, std::size_t& position) const {
        std::size_t draggedRow = 0;
        const bool found = findRow(dragged, draggedRow);
        if (!found || (parent.valid() && isInSubtree(parent, dragged))) {
            return false;
        }

        const Entity currentParent = getParentOfRow(draggedRow);
        if (currentParent != parent) {
            position = gap;
            return true;
        }
        // Roots have no order in Scene, so a root can only move under another entity.
        if (!parent.valid()) {
            return false;
        }
        const std::size_t current = getSiblingIndexOfRow(draggedRow);
        position = current < gap ? gap - 1 : gap;
        return position != current;
    }

    bool EditorSnapshot::resolveDrop(const std::span<const Entity> dragged, const Entity parent, const std::size_t gap, std::size_t& position) const {
        if (dragged.empty()) {
            return false;
        }

        bool anyChild = false;
        for (const Entity entity: dragged) {
            std::size_t row = 0;
            const bool found = findRow(entity, row);
            if (!found) {
                return false;
            }
            if (isInSubtree(parent, entity)) {
                return false;
            }
            if (hierarchy[row].depth > 0) {
                anyChild = true;
            }
        }

        // Roots have no order in Scene, so a drop at the root only moves the dragged entities that are children.
        if (!parent.valid()) {
            if (!anyChild) {
                return false;
            }
            position = 0;
            return true;
        }

        std::size_t parentRow = 0;
        const bool parentListed = findRow(parent, parentRow);
        if (!parentListed) {
            return false;
        }
        const std::uint32_t childDepth = hierarchy[parentRow].depth + 1;
        std::vector<Entity> children;
        for (std::size_t row = parentRow + 1; row < hierarchy.size() && hierarchy[row].depth >= childDepth; row++) {
            if (hierarchy[row].depth == childDepth) {
                children.push_back(hierarchy[row].entity);
            }
        }

        std::vector<Entity> remaining;
        std::size_t beforeGap = 0;
        for (std::size_t index = 0; index < children.size(); index++) {
            const Entity child = children[index];
            const bool isDragged = std::find(dragged.begin(), dragged.end(), child) != dragged.end();
            if (index < gap && !isDragged) {
                beforeGap++;
            }
            if (!isDragged) {
                remaining.push_back(child);
            }
        }

        position = beforeGap;
        remaining.insert(remaining.begin() + static_cast<std::ptrdiff_t>(position), dragged.begin(), dragged.end());
        return remaining != children;
    }

    void EditorSnapshot::collectTopmost(const std::span<const Entity> selection, std::vector<Entity>& out) const {
        out.clear();
        std::vector<bool> selectedPath;
        for (const HierarchyRow& row: hierarchy) {
            selectedPath.resize(row.depth + 1);
            const bool selected = std::find(selection.begin(), selection.end(), row.entity) != selection.end();
            const bool selectedAncestor = row.depth > 0 && selectedPath[row.depth - 1];
            selectedPath[row.depth] = selected || selectedAncestor;
            if (selected && !selectedAncestor) {
                out.push_back(row.entity);
            }
        }
    }
}
