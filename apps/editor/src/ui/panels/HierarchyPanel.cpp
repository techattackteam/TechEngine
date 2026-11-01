#include <edit/EditorCommands.hpp>
#include <edit/EditorSnapshot.hpp>
#include <ui/panels/HierarchyPanel.hpp>

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace TechEngine {
    static constexpr const char* ENTITY_PAYLOAD = "TE_HIERARCHY_ENTITY";
    static constexpr const char* ENTITY_MENU = "HierarchyEntityMenu";
    static constexpr float DROP_EDGE_RATIO = 0.25f;
    static constexpr float INSERTION_LINE_THICKNESS = 2.0f;

    static bool readEntitiesPayload(const ImGuiPayload* payload, std::vector<Entity>& out) {
        if (payload == nullptr || !payload->IsDataType(ENTITY_PAYLOAD) || payload->DataSize <= 0 || payload->DataSize % static_cast<int>(sizeof(Entity)) != 0) {
            return false;
        }
        out.resize(static_cast<std::size_t>(payload->DataSize) / sizeof(Entity));
        std::memcpy(out.data(), payload->Data, static_cast<std::size_t>(payload->DataSize));
        return true;
    }

    HierarchyPanel::HierarchyPanel(EditorCommands& commands) : Panel("Hierarchy"), m_commands(&commands) {
    }

    void HierarchyPanel::onDraw(const EditorSnapshot& snapshot) {
        collectVisibleRows(snapshot);
        adoptSnapshotSelection(snapshot);

        // The gap between rows moves inside each row as padding: the rows look as before, but they
        // touch, so a drag never hovers a gap that belongs to no drop target.
        const ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
        const ImVec2 padding = ImGui::GetStyle().FramePadding;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing.x, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padding.x, spacing.y * 0.5f));
        ImGuiMultiSelectIO* selectionIO = ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_None, static_cast<int>(m_selection.size()), static_cast<int>(m_visibleRows.size()));
        applySelectionRequests(snapshot, selectionIO);
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(m_visibleRows.size()));
        // A Shift range loses its anchor once the anchor's row is clipped.
        if (selectionIO->RangeSrcItem >= 0) {
            clipper.IncludeItemByIndex(static_cast<int>(selectionIO->RangeSrcItem));
        }
        std::vector<Entity> dragged;
        int draggedRow = 0;
        const bool dragging = readEntitiesPayload(ImGui::GetDragDropPayload(), dragged);
        // A drag source that is not submitted shows "..." as its preview, so the dragged row is never clipped.
        if (dragging && findVisibleRow(snapshot, m_dragSource, draggedRow)) {
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
                drawRow(snapshot, static_cast<std::size_t>(i));
            }
        }
        applySelectionRequests(snapshot, ImGui::EndMultiSelect());
        ImGui::PopStyleVar(2);

        drawEmptySpace(snapshot);
        drawMenu(snapshot);
        handleKeys(snapshot);
    }

    void HierarchyPanel::handleKeys(const EditorSnapshot& snapshot) {
        // Delete and F2 would otherwise fire while a name is being typed.
        const bool typing = m_renaming.valid() || ImGui::GetIO().WantTextInput;
        if (typing || !ImGui::IsWindowFocused() || m_selection.empty()) {
            return;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            std::vector<Entity> topmost;
            snapshot.collectTopmost(m_selection, topmost);
            m_commands->destroy(std::move(topmost));
        } else if (ImGui::IsKeyPressed(ImGuiKey_F2, false) && m_active.valid()) {
            beginRename(snapshot, m_active);
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

    void HierarchyPanel::adoptSnapshotSelection(const EditorSnapshot& snapshot) {
        if (snapshot.selectionSequence < m_sentSequence) {
            return;
        }
        m_selection = snapshot.selection;
        m_active = snapshot.active;
    }

    void HierarchyPanel::applySelectionRequests(const EditorSnapshot& snapshot, ImGuiMultiSelectIO* io) {
        if (io->Requests.Size == 0) {
            return;
        }

        const std::vector<Entity> previous = m_selection;
        const Entity previousActive = m_active;
        const ImGuiSelectionUserData visibleCount = static_cast<ImGuiSelectionUserData>(m_visibleRows.size());
        ImGuiSelectionUserData clickedRow = -1;
        for (const ImGuiSelectionRequest& request: io->Requests) {
            if (request.Type == ImGuiSelectionRequestType_SetAll) {
                m_selection.clear();
                if (request.Selected) {
                    for (const std::size_t rowIndex: m_visibleRows) {
                        m_selection.push_back(snapshot.hierarchy[rowIndex].entity);
                    }
                }
                continue;
            }
            if (request.Type != ImGuiSelectionRequestType_SetRange) {
                continue;
            }

            for (ImGuiSelectionUserData visible = request.RangeFirstItem; visible <= request.RangeLastItem && visible < visibleCount; visible++) {
                const Entity entity = snapshot.hierarchy[m_visibleRows[static_cast<std::size_t>(visible)]].entity;
                const auto found = std::ranges::find(m_selection, entity);
                if (request.Selected && found == m_selection.end()) {
                    m_selection.push_back(entity);
                } else if (!request.Selected && found != m_selection.end()) {
                    m_selection.erase(found);
                }
            }
            // The range runs from the Shift anchor to the clicked row, and RangeDirection says which end that is.
            clickedRow = request.RangeDirection > 0 ? request.RangeLastItem : request.RangeFirstItem;
        }

        const bool clickedShown = clickedRow >= 0 && clickedRow < visibleCount;
        const Entity clicked = clickedShown ? snapshot.hierarchy[m_visibleRows[static_cast<std::size_t>(clickedRow)]].entity : Entity{};
        if (clicked.valid() && isSelected(clicked)) {
            m_active = clicked;
        } else if (!isSelected(m_active)) {
            m_active = m_selection.empty() ? Entity{} : m_selection.back();
        }

        const bool changed = m_active != previousActive || !std::ranges::is_permutation(m_selection, previous);
        if (changed) {
            sendSelection();
        }
    }

    void HierarchyPanel::sendSelection() {
        m_sentSequence++;
        m_commands->select(m_selection, m_active, m_sentSequence);
    }

    bool HierarchyPanel::resolveDraggedDrop(const EditorSnapshot& snapshot, const std::vector<Entity>& dragged, const Entity parent, const std::size_t gap, std::size_t& position) {
        if (dragged.size() == 1) {
            return snapshot.resolveDrop(dragged[0], parent, gap, position);
        }
        return snapshot.resolveDrop(std::span<const Entity>(dragged), parent, gap, position);
    }

    bool HierarchyPanel::isSelected(const Entity entity) const {
        return std::ranges::find(m_selection, entity) != m_selection.end();
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

    void HierarchyPanel::drawRow(const EditorSnapshot& snapshot, const std::size_t visibleRow) {
        const std::size_t rowIndex = m_visibleRows[visibleRow];
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
            menuRequested = drawTreeNode(snapshot, rowIndex, visibleRow, rowStartX);
        }

        if (indent > 0.0f) {
            ImGui::Unindent(indent);
        }
        ImGui::PopID();
        ImGui::PopID();
        // Opened outside the row's ID scope, so the one menu drawMenu() submits finds it even after the row is clipped.
        if (menuRequested) {
            m_menuTarget = row.entity;
            ImGui::OpenPopup(ENTITY_MENU);
        }
    }

    bool HierarchyPanel::drawTreeNode(const EditorSnapshot& snapshot, const std::size_t rowIndex, const std::size_t visibleRow, const float rowStartX) {
        const HierarchyRow& row = snapshot.hierarchy[rowIndex];
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_FramePadding;
        if (row.childCount == 0) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        } else {
            ImGui::SetNextItemOpen(!m_collapsed.contains(row.entity), ImGuiCond_Always);
        }
        const bool selected = isSelected(row.entity);
        if (selected) {
            flags |= ImGuiTreeNodeFlags_Selected;
            // HeaderHovered is a gray, so a hovered selected row would otherwise lose its selection color.
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyle().Colors[ImGuiCol_Header]);
        }

        ImGui::SetNextItemSelectionUserData(static_cast<ImGuiSelectionUserData>(visibleRow));
        const bool open = ImGui::TreeNodeEx("##row", flags, "%s", row.name.c_str());
        if (selected) {
            ImGui::PopStyleColor();
        }
        if (ImGui::IsItemToggledOpen()) {
            if (open) {
                m_collapsed.erase(row.entity);
            } else {
                m_collapsed.insert(row.entity);
            }
        }
        const bool menuRequested = ImGui::IsItemClicked(ImGuiMouseButton_Right);

        if (ImGui::BeginDragDropSource()) {
            m_dragSource = row.entity;
            std::vector<Entity> dragged;
            if (selected) {
                snapshot.collectTopmost(m_selection, dragged);
            } else {
                dragged.push_back(row.entity);
            }
            ImGui::SetDragDropPayload(ENTITY_PAYLOAD, dragged.data(), dragged.size() * sizeof(Entity));
            std::size_t draggedRow = 0;
            const bool single = dragged.size() == 1 && snapshot.findRow(dragged[0], draggedRow);
            if (single) {
                ImGui::TextUnformatted(snapshot.hierarchy[draggedRow].name.c_str());
            } else {
                ImGui::Text("%zu entities", dragged.size());
            }
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
        std::vector<Entity> dragged;
        const bool dragging = readEntitiesPayload(ImGui::GetDragDropPayload(), dragged);
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
        const bool allowed = resolveDraggedDrop(snapshot, dragged, parent, gap, position);
        if (!allowed) {
            return;
        }
        if (insertion) {
            const float lineX = rowStartX + static_cast<float>(lineDepth) * ImGui::GetStyle().IndentSpacing;
            ImGui::GetWindowDrawList()->AddLine(ImVec2(lineX, lineY), ImVec2(rowMax.x, lineY), ImGui::GetColorU32(ImGuiCol_DragDropTarget), INSERTION_LINE_THICKNESS);
        }
        acceptDrop(std::move(dragged), parent, position, !insertion);
    }

    void HierarchyPanel::drawEmptySpace(const EditorSnapshot& snapshot) {
        const ImVec2 available = ImGui::GetContentRegionAvail();
        // InvisibleButton asserts on a zero size, which a docked panel shrunk to nothing would give it.
        ImGui::InvisibleButton("##emptySpace", ImVec2(std::max(available.x, 1.0f), std::max(available.y, ImGui::GetFrameHeight())));
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !m_selection.empty()) {
            m_selection.clear();
            m_active = {};
            sendSelection();
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            m_menuTarget = Entity{};
            ImGui::OpenPopup(ENTITY_MENU);
        }
        if (ImGui::BeginDragDropTarget()) {
            std::vector<Entity> dragged;
            std::size_t position = 0;
            const bool dragging = readEntitiesPayload(ImGui::GetDragDropPayload(), dragged);
            const bool allowed = dragging && resolveDraggedDrop(snapshot, dragged, Entity{}, 0, position);
            if (allowed) {
                const ImVec2 spaceMin = ImGui::GetItemRectMin();
                const ImVec2 spaceMax = ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddLine(spaceMin, ImVec2(spaceMax.x, spaceMin.y), ImGui::GetColorU32(ImGuiCol_DragDropTarget), INSERTION_LINE_THICKNESS);
                acceptDrop(std::move(dragged), Entity{}, position, false);
            }
            ImGui::EndDragDropTarget();
        }
    }

    void HierarchyPanel::acceptDrop(std::vector<Entity> dragged, const Entity parent, const std::size_t position, const bool highlightRow) {
        const ImGuiDragDropFlags flags = highlightRow ? ImGuiDragDropFlags_None : ImGuiDragDropFlags_AcceptNoDrawDefaultRect;
        const ImGuiPayload* accepted = ImGui::AcceptDragDropPayload(ENTITY_PAYLOAD, flags);
        if (accepted == nullptr) {
            return;
        }
        m_commands->reparent(std::move(dragged), parent, position);
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
                std::vector<Entity> topmost;
                snapshot.collectTopmost(m_selection, topmost);
                m_commands->destroy(std::move(topmost));
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
