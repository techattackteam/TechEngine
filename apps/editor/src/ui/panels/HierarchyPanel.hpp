#pragma once

#include <TechEngine/core/scene/Entity.hpp>

#include <ui/panels/Panel.hpp>
#include <ui/widgets/TextEditField.hpp>

#include <cstddef>
#include <unordered_set>
#include <vector>

namespace TechEngine {
    class EditorCommands;
    struct HierarchyRow;

    class HierarchyPanel final : public Panel {
    private:
        EditorCommands* m_commands = nullptr;
        std::unordered_set<Entity> m_collapsed;
        std::vector<std::size_t> m_visibleRows;
        Entity m_menuTarget;
        Entity m_renaming;
        TextEditField m_renameField;

    public:
        explicit HierarchyPanel(EditorCommands& commands);

    protected:
        void onDraw(const EditorSnapshot& snapshot) override;

    private:
        void collectVisibleRows(const EditorSnapshot& snapshot);

        bool findVisibleRow(const EditorSnapshot& snapshot, Entity entity, int& out) const;

        void drawRow(const EditorSnapshot& snapshot, std::size_t rowIndex);

        bool drawTreeNode(const EditorSnapshot& snapshot, std::size_t rowIndex, float rowStartX);

        void drawRenameField(const HierarchyRow& row);

        void acceptRowDrop(const EditorSnapshot& snapshot, std::size_t rowIndex, float rowStartX);

        void drawEmptySpace(const EditorSnapshot& snapshot);

        void acceptDrop(Entity dragged, Entity parent, std::size_t position, bool highlightRow);

        void drawMenu(const EditorSnapshot& snapshot);

        void handleKeys(const EditorSnapshot& snapshot);

        void beginRename(const EditorSnapshot& snapshot, Entity entity);

        void createChild(Entity parent);
    };
}
