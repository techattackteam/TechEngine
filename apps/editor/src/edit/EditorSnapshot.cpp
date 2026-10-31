#include <edit/EditorSnapshot.hpp>

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
}
