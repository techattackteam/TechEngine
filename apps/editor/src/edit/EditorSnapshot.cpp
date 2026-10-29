#include <edit/EditorSnapshot.hpp>

namespace TechEngine {
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
}
