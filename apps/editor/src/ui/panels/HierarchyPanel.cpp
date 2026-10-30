#include <edit/EditorCommands.hpp>
#include <edit/EditorSnapshot.hpp>
#include <ui/panels/HierarchyPanel.hpp>

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace TechEngine {
    static constexpr const char* ENTITY_PAYLOAD = "TE_HIERARCHY_ENTITY";
    static constexpr const char* ENTITY_MENU = "HierarchyEntityMenu";
    static constexpr float DROP_EDGE_RATIO = 0.25f;
    static constexpr float INSERTION_LINE_THICKNESS = 2.0f;

    static bool readEntityPayload(const ImGuiPayload* payload, Entity& out) {
        if (payload == nullptr || !payload->IsDataType(ENTITY_PAYLOAD) || payload->DataSize != static_cast<int>(sizeof(Entity))) {
            return false;
        }
        std::memcpy(&out, payload->Data, sizeof(Entity));
        return true;
    }

    HierarchyPanel::HierarchyPanel(EditorCommands& commands) : Panel("Hierarchy"), m_commands(&commands) {
    }

    void HierarchyPanel::onDraw(const EditorSnapshot& snapshot) {
        collectVisibleRows(snapshot);

        // The gap between rows moves inside each row as padding: the rows look as before, but they
        // touch, so a drag never hovers a gap that belongs to no drop target.
        const ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
        const ImVec2 padding = ImGui::GetStyle().FramePadding;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing.x, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padding.x, spacing.y * 0.5f));
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(m_visibleRows.size()));
        Entity dragged;
        int draggedRow = 0;
        const bool dragging = readEntityPayload(ImGui::GetDragDropPayload(), dragged);
        // A drag source that is not submitted shows "..." as its preview, so the dragged row is never clipped.
        if (dragging && findVisibleRow(snapshot, dragged, draggedRow)) {
            clipper.IncludeItemByIndex(draggedRow);
        }
        int renamingRow = 0;
        if (m_renaming.valid()) {
            const bool shown = findVisibleRow(snapshot, m_renaming, renamingRow);
            if (shown) {
                clipper.IncludeItemByIndex(renamingRow);
            } else {
                m_renaming = {};
            }
        }
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                drawRow(snapshot, m_visibleRows[static_cast<std::size_t>(i)]);
            }
        }
        ImGui::PopStyleVar(2);

        drawEmptySpace(snapshot);
        drawMenu(snapshot);
        handleKeys(snapshot);
    }

    void HierarchyPanel::handleKeys(const EditorSnapshot& snapshot) {
        // Delete and F2 would otherwise fire while a name is being typed.
        const bool typing = m_renaming.valid() || ImGui::GetIO().WantTextInput;
        if (typing || !ImGui::IsWindowFocused() || !snapshot.selected.valid()) {
            return;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            m_commands->destroy(snapshot.selected);
        } else if (ImGui::IsKeyPressed(ImGuiKey_F2, false)) {
            beginRename(snapshot, snapshot.selected);
        }
    }

    void HierarchyPanel::collectVisibleRows(const EditorSnapshot& snapshot) {
        m_visibleRows.clear();
        const std::vector<HierarchyRow>& rows = snapshot.hierarchy;
        std::size_t index = 0;
        while (index < rows.size()) {
            const HierarchyRow& row = rows[index];
            m_visibleRows.push_back(index);
            index++;
            if (m_collapsed.contains(row.entity)) {
                while (index < rows.size() && rows[index].depth > row.depth) {
                    index++;
                }
            }
        }
    }

    bool HierarchyPanel::findVisibleRow(const EditorSnapshot& snapshot, const Entity entity, int& out) const {
        for (std::size_t visible = 0; visible < m_visibleRows.size(); visible++) {
            if (snapshot.hierarchy[m_visibleRows[visible]].entity == entity) {
                out = static_cast<int>(visible);
                return true;
            }
        }
        return false;
    }

    void HierarchyPanel::drawRow(const EditorSnapshot& snapshot, const std::size_t rowIndex) {
        const HierarchyRow& row = snapshot.hierarchy[rowIndex];
        ImGui::PushID(static_cast<int>(row.entity.index));
        ImGui::PushID(static_cast<int>(row.entity.generation));
        const float rowStartX = ImGui::GetCursorScreenPos().x;
        const float indent = static_cast<float>(row.depth) * ImGui::GetStyle().IndentSpacing;
        // Indent(0) indents by the default spacing, so a root must skip it.
        if (indent > 0.0f) {
            ImGui::Indent(indent);
        }

        bool menuRequested = false;
        if (row.entity == m_renaming) {
            drawRenameField(row);
        } else {
            menuRequested = drawTreeNode(snapshot, rowIndex, rowStartX);
        }

        if (indent > 0.0f) {
            ImGui::Unindent(indent);
        }
        ImGui::PopID();
        ImGui::PopID();
        // Opened outside the row's ID scope, so the one menu drawMenu() submits finds it even after the row is clipped.
        if (menuRequested) {
            m_menuTarget = row.entity;
            if (row.entity != snapshot.selected) {
                m_commands->select(row.entity);
            }
            ImGui::OpenPopup(ENTITY_MENU);
        }
    }

    bool HierarchyPanel::drawTreeNode(const EditorSnapshot& snapshot, const std::size_t rowIndex, const float rowStartX) {
        const HierarchyRow& row = snapshot.hierarchy[rowIndex];
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_FramePadding;
        if (row.childCount == 0) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        } else {
            ImGui::SetNextItemOpen(!m_collapsed.contains(row.entity), ImGuiCond_Always);
        }
        if (row.entity == snapshot.selected) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        const bool open = ImGui::TreeNodeEx("##row", flags, "%s", row.name.c_str());
        if (ImGui::IsItemToggledOpen()) {
            if (open) {
                m_collapsed.erase(row.entity);
            } else {
                m_collapsed.insert(row.entity);
            }
        } else if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && row.entity != snapshot.selected) {
            m_commands->select(row.entity);
        }
        const bool menuRequested = ImGui::IsItemClicked(ImGuiMouseButton_Right);

        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload(ENTITY_PAYLOAD, &row.entity, sizeof(Entity));
            ImGui::TextUnformatted(row.name.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            acceptRowDrop(snapshot, rowIndex, rowStartX);
            ImGui::EndDragDropTarget();
        }
        return menuRequested;
    }

    void HierarchyPanel::drawRenameField(const HierarchyRow& row) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetTreeNodeToLabelSpacing());
        ImGui::SetNextItemWidth(-FLT_MIN);
        std::string name;
        const bool finished = m_renameField.draw("##rename", row.name, name);
        if (!finished) {
            return;
        }

        // Escape restores the old name before the field deactivates, so a cancelled rename sends nothing.
        if (name != row.name) {
            m_commands->rename(m_renaming, name);
        }
        m_renaming = {};
    }

    void HierarchyPanel::acceptRowDrop(const EditorSnapshot& snapshot, const std::size_t rowIndex, const float rowStartX) {
        Entity dragged;
        const bool dragging = readEntityPayload(ImGui::GetDragDropPayload(), dragged);
        if (!dragging) {
            return;
        }

        const HierarchyRow& row = snapshot.hierarchy[rowIndex];
        const ImVec2 rowMin = ImGui::GetItemRectMin();
        const ImVec2 rowMax = ImGui::GetItemRectMax();
        const float edge = (rowMax.y - rowMin.y) * DROP_EDGE_RATIO;
        const float mouseY = ImGui::GetMousePos().y;

        Entity parent = row.entity;
        std::size_t gap = 0;
        std::uint32_t lineDepth = row.depth;
        float lineY = 0.0f;
        bool insertion = true;
        if (mouseY < rowMin.y + edge) {
            parent = snapshot.getParentOfRow(rowIndex);
            gap = snapshot.getSiblingIndexOfRow(rowIndex);
            lineY = rowMin.y;
        } else if (mouseY > rowMax.y - edge) {
            lineY = rowMax.y;
            const bool showsChildren = row.childCount > 0 && !m_collapsed.contains(row.entity);
            if (showsChildren) {
                gap = 0;
                lineDepth = row.depth + 1;
            } else {
                parent = snapshot.getParentOfRow(rowIndex);
                gap = snapshot.getSiblingIndexOfRow(rowIndex) + 1;
            }
        } else {
            insertion = false;
        }

        std::size_t position = 0;
        const bool allowed = snapshot.resolveDrop(dragged, parent, gap, position);
        if (!allowed) {
            return;
        }
        if (insertion) {
            const float lineX = rowStartX + static_cast<float>(lineDepth) * ImGui::GetStyle().IndentSpacing;
            ImGui::GetWindowDrawList()->AddLine(ImVec2(lineX, lineY), ImVec2(rowMax.x, lineY), ImGui::GetColorU32(ImGuiCol_DragDropTarget), INSERTION_LINE_THICKNESS);
        }
        acceptDrop(dragged, parent, position, !insertion);
    }

    void HierarchyPanel::drawEmptySpace(const EditorSnapshot& snapshot) {
        const ImVec2 available = ImGui::GetContentRegionAvail();
        // InvisibleButton asserts on a zero size, which a docked panel shrunk to nothing would give it.
        ImGui::InvisibleButton("##emptySpace", ImVec2(std::max(available.x, 1.0f), std::max(available.y, ImGui::GetFrameHeight())));
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && snapshot.selected.valid()) {
            m_commands->select(Entity{});
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            m_menuTarget = Entity{};
            ImGui::OpenPopup(ENTITY_MENU);
        }
        if (ImGui::BeginDragDropTarget()) {
            Entity dragged;
            std::size_t position = 0;
            const bool dragging = readEntityPayload(ImGui::GetDragDropPayload(), dragged);
            const bool allowed = dragging && snapshot.resolveDrop(dragged, Entity{}, 0, position);
            if (allowed) {
                const ImVec2 spaceMin = ImGui::GetItemRectMin();
                const ImVec2 spaceMax = ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddLine(spaceMin, ImVec2(spaceMax.x, spaceMin.y), ImGui::GetColorU32(ImGuiCol_DragDropTarget), INSERTION_LINE_THICKNESS);
                acceptDrop(dragged, Entity{}, position, false);
            }
            ImGui::EndDragDropTarget();
        }
    }

    void HierarchyPanel::acceptDrop(const Entity dragged, const Entity parent, const std::size_t position, const bool highlightRow) {
        const ImGuiDragDropFlags flags = highlightRow ? ImGuiDragDropFlags_None : ImGuiDragDropFlags_AcceptNoDrawDefaultRect;
        const ImGuiPayload* accepted = ImGui::AcceptDragDropPayload(ENTITY_PAYLOAD, flags);
        if (accepted == nullptr) {
            return;
        }
        m_commands->reparent(dragged, parent, position);
        if (parent.valid()) {
            m_collapsed.erase(parent);
        }
    }

    void HierarchyPanel::drawMenu(const EditorSnapshot& snapshot) {
        if (!ImGui::BeginPopup(ENTITY_MENU)) {
            return;
        }
        if (m_menuTarget.valid()) {
            if (ImGui::MenuItem("Create Child")) {
                createChild(m_menuTarget);
            }
            if (ImGui::MenuItem("Rename", "F2")) {
                beginRename(snapshot, m_menuTarget);
            }
            if (ImGui::MenuItem("Delete", "Del")) {
                m_commands->destroy(m_menuTarget);
            }
        } else if (ImGui::MenuItem("Create")) {
            m_commands->create(Entity{});
        }
        ImGui::EndPopup();
    }

    void HierarchyPanel::beginRename(const EditorSnapshot& snapshot, const Entity entity) {
        std::size_t rowIndex = 0;
        const bool found = snapshot.findRow(entity, rowIndex);
        if (!found) {
            return;
        }
        m_renaming = entity;
        m_renameField.requestFocus();
    }

    void HierarchyPanel::createChild(const Entity parent) {
        m_commands->create(parent);
        m_collapsed.erase(parent);
    }
}
