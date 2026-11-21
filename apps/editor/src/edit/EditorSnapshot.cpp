#include <TechEngine/core/resources/Side.hpp>

#include <edit/EditorSnapshot.hpp>

#include <algorithm>
#include <cctype>

namespace TechEngine {
    bool EditorSnapshot::hasUnsavedEdits() const {
        if (editRevision != savedRevision) {
            return true;
        }
        return false;
    }

    bool EditorSnapshot::hasMixedNames() const {
        const std::string* firstName = nullptr;
        for (const Entity entity: selection) {
            std::size_t row = 0;
            const bool found = findRow(entity, row);
            if (!found) {
                continue;
            }
            const std::string& rowName = hierarchy[row].name;
            if (firstName == nullptr) {
                firstName = &rowName;
                continue;
            }
            if (rowName != *firstName) {
                return true;
            }
        }
        return false;
    }

    bool EditorSnapshot::isInSubtree(const Entity entity, const Entity root) const {
        if (!root.valid()) {
            return false;
        }

        std::size_t rootIndex = 0;
        const bool found = findRow(root, rootIndex);
        if (!found) {
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

    void EditorSnapshot::collectAssetsOfType(const StringId type, std::vector<std::size_t>& out) const {
        out.clear();
        if (type == StringId{}) {
            return;
        }
        for (std::size_t i = 0; i < assets.size(); i++) {
            if (assets[i].typeTag == type && assets[i].side != Side::Server) {
                out.push_back(i);
            }
        }
    }

    void EditorSnapshot::collectFolderContents(const std::string_view folder, std::vector<std::size_t>& out) const {
        out.clear();
        const std::string prefix = std::string(folder) + "/";
        for (std::size_t index = 0; index < listing.size(); index++) {
            const std::string_view path = listing[index].path;
            if (path.size() <= prefix.size() || path.substr(0, prefix.size()) != prefix) {
                continue;
            }
            if (path.find('/', prefix.size()) == std::string_view::npos) {
                out.push_back(index);
            }
        }

        const auto lower = [](const char character) {
            return static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        };
        const auto leaf = [](const std::string_view path) {
            return path.substr(path.rfind('/') + 1);
        };
        std::ranges::stable_sort(out, [this, &lower, &leaf](const std::size_t left, const std::size_t right) {
            const bool leftFolder = listing[left].kind == ListedKind::Folder;
            const bool rightFolder = listing[right].kind == ListedKind::Folder;
            if (leftFolder != rightFolder) {
                return leftFolder;
            }

            const std::string_view leftName = leaf(listing[left].path);
            const std::string_view rightName = leaf(listing[right].path);
            const std::size_t length = std::min(leftName.size(), rightName.size());
            for (std::size_t i = 0; i < length; i++) {
                const char leftCharacter = lower(leftName[i]);
                const char rightCharacter = lower(rightName[i]);
                if (leftCharacter != rightCharacter) {
                    return leftCharacter < rightCharacter;
                }
            }
            return leftName.size() < rightName.size();
        });
    }

    bool EditorSnapshot::findNearestFolder(const std::string_view folder, std::string& out) const {
        out.clear();
        std::string candidate(folder);
        while (!candidate.empty()) {
            const bool isSideFolder = std::ranges::any_of(sideFolders, [&candidate](const SideFolder& sideFolder) {
                return sideFolder.path == candidate;
            });
            const bool isListedFolder = std::ranges::any_of(listing, [&candidate](const ListedEntry& entry) {
                return entry.kind == ListedKind::Folder && entry.path == candidate;
            });
            if (isSideFolder || isListedFolder) {
                out = candidate;
                return true;
            }

            const std::size_t separator = candidate.rfind('/');
            if (separator == std::string::npos) {
                break;
            }
            candidate.resize(separator);
        }
        return false;
    }

    void EditorSnapshot::collectBreadcrumbs(const std::string_view folder, std::vector<std::string>& out) const {
        out.clear();
        std::string side;
        for (const SideFolder& sideFolder: sideFolders) {
            const std::string_view path = sideFolder.path;
            const bool isAncestor = folder == path || (folder.size() > path.size() && folder.substr(0, path.size()) == path && folder[path.size()] == '/');
            if (isAncestor && path.size() > side.size()) {
                side = sideFolder.path;
            }
        }
        if (side.empty()) {
            return;
        }

        out.push_back(side);
        std::size_t start = side.size() + 1;
        while (start < folder.size()) {
            const std::size_t separator = folder.find('/', start);
            const std::size_t end = separator == std::string_view::npos ? folder.size() : separator;
            out.emplace_back(folder.substr(0, end));
            if (separator == std::string_view::npos) {
                break;
            }
            start = separator + 1;
        }
    }
}
