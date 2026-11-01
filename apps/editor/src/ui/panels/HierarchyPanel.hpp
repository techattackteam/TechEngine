#pragma once

#include <TechEngine/core/scene/Entity.hpp>

#include <ui/panels/Panel.hpp>
#include <ui/widgets/TextEditField.hpp>

#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>

struct ImGuiMultiSelectIO;

namespace TechEngine {
    class EditorCommands;
    struct HierarchyRow;

    class HierarchyPanel final : public Panel {
    private:
        EditorCommands* m_commands = nullptr;
        std::unordered_set<Entity> m_collapsed;
        std::vector<std::size_t> m_visibleRows;
        // Main's own set: the rows draw it, and it replaces the snapshot's until the snapshot carries m_sentSequence.
        std::vector<Entity> m_selection;
        Entity m_active;
        std::uint64_t m_sentSequence = 0;
        Entity m_dragSource;
        Entity m_menuTarget;
        Entity m_renaming;
        TextEditField m_renameField;

    public:
        explicit HierarchyPanel(EditorCommands& commands);

    protected:
        void onDraw(const EditorSnapshot& snapshot) override;

    private:
        void collectVisibleRows(const EditorSnapshot& snapshot);

        void adoptSnapshotSelection(const EditorSnapshot& snapshot);

        void applySelectionRequests(const EditorSnapshot& snapshot, ImGuiMultiSelectIO* io);

        void sendSelection();

        bool isSelected(Entity entity) const;

        static bool resolveDraggedDrop(const EditorSnapshot& snapshot, const std::vector<Entity>& dragged, Entity parent, std::size_t gap, std::size_t& position);

        bool findVisibleRow(const EditorSnapshot& snapshot, Entity entity, int& out) const;

        void drawRow(const EditorSnapshot& snapshot, std::size_t visibleRow);

        bool drawTreeNode(const EditorSnapshot& snapshot, std::size_t rowIndex, std::size_t visibleRow, float rowStartX);

        void drawRenameField(const HierarchyRow& row);

        void acceptRowDrop(const EditorSnapshot& snapshot, std::size_t rowIndex, float rowStartX);

        void drawEmptySpace(const EditorSnapshot& snapshot);

        void acceptDrop(std::vector<Entity> dragged, Entity parent, std::size_t position, bool highlightRow);

        void drawMenu(const EditorSnapshot& snapshot);

        void handleKeys(const EditorSnapshot& snapshot);

        void beginRename(const EditorSnapshot& snapshot, Entity entity);

        void createChild(Entity parent);
    };
}
