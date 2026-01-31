#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Panel.hpp"
#include <imgui_internal.h>

namespace TechEngine {

    /**
     * @brief Configuration flags for hierarchy panel behavior
     */
    struct HierarchyPanelConfig {
        bool allowReparenting = true;      // Can drag-drop to change parent
        bool allowReordering = true;       // Can reorder siblings
        bool allowCreation = true;         // Can create new nodes
        bool allowDeletion = true;         // Can delete nodes
        bool allowRenaming = true;         // Can rename nodes
        bool showDropZones = true;         // Show drop zones between items
        bool preserveWorldTransform = true; // Preserve world transform on reparent
    };

    /**
     * @brief Cache structure for hierarchy nodes - stores display state
     * @tparam TNodeId The type used to identify nodes (e.g., Entity, Widget*)
     */
    template<typename TNodeId>
    struct HierarchyNodeCache {
        TNodeId id{};
        TNodeId parent{};
        std::vector<TNodeId> children;
        std::string name;
        int depth = 0;
        bool isExpanded = true;
        bool isLoaded = false;
        size_t childCount = 0;

        HierarchyNodeCache() = default;
        HierarchyNodeCache(TNodeId nodeId, TNodeId parentId, std::string nodeName, int nodeDepth)
            : id(nodeId), parent(parentId), name(std::move(nodeName)), depth(nodeDepth), isLoaded(true) {}
    };

    /**
     * @brief Base class for hierarchy panels with common tree display logic
     * @tparam TNodeId The type used to identify nodes
     * @tparam TInvalidId A constexpr function or value representing an invalid node ID
     */
    template<typename TNodeId>
    class HierarchyPanelBase : public Panel {
    public:
        using NodeCache = HierarchyNodeCache<TNodeId>;

    protected:
        // Configuration
        HierarchyPanelConfig m_config;

        // Cache
        std::unordered_map<TNodeId, NodeCache> m_nodeCache;
        std::vector<TNodeId> m_rootNodes;
        std::unordered_set<TNodeId> m_dirtyNodes;

        // State
        TNodeId m_selectedNode{};
        bool m_isDirty = true;
        bool m_isFullRebuildNeeded = true;
        bool m_isNodeHovered = false;

        // Lazy loading
        static constexpr size_t LAZY_LOAD_THRESHOLD = 1000;
        size_t m_totalNodeCount = 0;
        bool m_useLazyLoading = false;

        // Drag & drop payload name - derived classes should set this
        std::string m_dragDropPayloadName = "HIERARCHY_NODE";

    public:
        HierarchyPanelBase(SystemsRegistry& editorSystemsRegistry, const HierarchyPanelConfig& config = {})
            : Panel(editorSystemsRegistry), m_config(config) {}

        ~HierarchyPanelBase() override = default;

        // Panel interface
        void onInit() override {
            onHierarchyInit();
        }

        void onUpdate() override {
            m_isNodeHovered = false;

            drawHierarchy();

            handleEmptySpaceClick();
            handleEmptySpaceContextMenu();

            processQueuedOperations();
        }

        // Selection
        TNodeId getSelectedNode() const { return m_selectedNode; }
        void setSelectedNode(TNodeId node) { m_selectedNode = node; }
        void clearSelection() { m_selectedNode = getInvalidNodeId(); }

        // Dirty state management
        void markDirty() { m_isDirty = true; }
        void markFullRebuildNeeded() { m_isDirty = true; m_isFullRebuildNeeded = true; }

    protected:

        virtual TNodeId getInvalidNodeId() const = 0;

        virtual bool isValidNodeId(TNodeId id) const = 0;

        virtual void rebuildCache() = 0;

        virtual std::string getNodeName(TNodeId id) const = 0;

        virtual TNodeId getNodeParent(TNodeId id) const = 0;

        virtual std::vector<TNodeId> getNodeChildren(TNodeId id) const = 0;

        virtual void onSelectionChanged(TNodeId newSelection) = 0;

        virtual void onHierarchyInit() {}

        virtual void drawEmptySpaceContextMenu() {}

        virtual void drawNodeContextMenu(TNodeId node) {}

        virtual void reparentNode(TNodeId node, TNodeId newParent, TNodeId insertBefore) {}

        virtual void processQueuedOperations() {}

        virtual bool isDescendantOf(TNodeId potentialAncestor, TNodeId node) const {
            if (!isValidNodeId(node)) return false;

            TNodeId current = node;
            while (isValidNodeId(current)) {
                if (current == potentialAncestor) return true;
                auto it = m_nodeCache.find(current);
                if (it == m_nodeCache.end()) break;
                current = it->second.parent;
            }
            return false;
        }

        void drawHierarchy() {
            // Rebuild cache if needed
            if (m_isDirty) {
                if (m_isFullRebuildNeeded) {
                    rebuildCache();
                    m_isFullRebuildNeeded = false;
                } else {
                    updateDirtyNodes();
                }
                m_isDirty = false;
            }

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 10.0f);

            // Draw root nodes and their children
            for (size_t i = 0; i < m_rootNodes.size(); ++i) {
                TNodeId rootNode = m_rootNodes[i];

                if (m_config.showDropZones && m_config.allowReparenting) {
                    drawDropZone(getInvalidNodeId(), rootNode, 0);
                }

                drawNode(rootNode, 0);
            }

            // Final drop zone at root level
            if (m_config.showDropZones && m_config.allowReparenting) {
                drawDropZone(getInvalidNodeId(), getInvalidNodeId(), 0);
            }

            ImGui::PopStyleVar(3);
        }

        void drawNode(TNodeId nodeId, int depth) {
            auto cacheIt = m_nodeCache.find(nodeId);
            if (cacheIt == m_nodeCache.end()) return;

            NodeCache& cache = cacheIt->second;

            ImGui::PushID(static_cast<int>(std::hash<TNodeId>{}(nodeId)));

            ImGui::Indent((depth + 1) * ImGui::GetStyle().IndentSpacing);

            const bool isSelected = (m_selectedNode == nodeId);
            const bool hasChildren = !cache.children.empty();

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                       ImGuiTreeNodeFlags_SpanAvailWidth |
                                       ImGuiTreeNodeFlags_NoTreePushOnOpen;

            if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;
            if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;

            if (hasChildren) {
                ImGui::SetNextItemOpen(cache.isExpanded, ImGuiCond_Always);
            }

            bool nodeOpen = ImGui::TreeNodeEx("##Node", flags, "%s", cache.name.c_str());
            m_isNodeHovered |= ImGui::IsItemHovered();

            // Handle selection
            if (ImGui::IsItemClicked(0) && !ImGui::IsItemToggledOpen()) {
                m_selectedNode = nodeId;
                onSelectionChanged(nodeId);
            }

            // Handle expand/collapse
            if (ImGui::IsItemToggledOpen()) {
                cache.isExpanded = nodeOpen;
            }

            // Drag source
            if (m_config.allowReparenting && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload(m_dragDropPayloadName.c_str(), &nodeId, sizeof(TNodeId));
                ImGui::Text("%s", cache.name.c_str());
                ImGui::EndDragDropSource();
            }

            // Drop target - dropping onto a node makes it a child
            if (m_config.allowReparenting && ImGui::BeginDragDropTarget()) {
                const ImGuiPayload* payload = ImGui::GetDragDropPayload();
                if (payload && payload->IsDataType(m_dragDropPayloadName.c_str())) {
                    TNodeId draggedNode = *static_cast<const TNodeId*>(payload->Data);

                    if (draggedNode != nodeId && !isDescendantOf(draggedNode, nodeId)) {
                        ImGui::GetWindowDrawList()->AddRect(
                            ImGui::GetItemRectMin(),
                            ImGui::GetItemRectMax(),
                            ImGui::GetColorU32(ImGuiCol_DragDropTarget),
                            0.0f, 0, 2.0f
                        );

                        if (ImGui::AcceptDragDropPayload(m_dragDropPayloadName.c_str(), ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                            TNodeId firstChild = cache.children.empty() ? getInvalidNodeId() : cache.children.front();
                            reparentNode(draggedNode, nodeId, firstChild);
                            markFullRebuildNeeded();
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // Node context menu
            if (ImGui::BeginPopupContextItem("##NodeContextMenu")) {
                m_selectedNode = nodeId;
                drawNodeContextMenu(nodeId);
                ImGui::EndPopup();
            }

            ImGui::Unindent((depth + 1) * ImGui::GetStyle().IndentSpacing);

            // Draw children if expanded
            if (cache.isExpanded && hasChildren) {
                for (size_t i = 0; i < cache.children.size(); ++i) {
                    TNodeId childNode = cache.children[i];

                    if (m_config.showDropZones && m_config.allowReordering) {
                        drawDropZone(nodeId, childNode, depth + 1);
                    }

                    drawNode(childNode, depth + 1);
                }

                if (m_config.showDropZones && m_config.allowReordering) {
                    drawDropZone(nodeId, getInvalidNodeId(), depth + 1);
                }
            }

            ImGui::PopID();
        }

        void drawDropZone(TNodeId parentNode, TNodeId targetBeforeNode, int depth) {
            constexpr float dropZoneHeight = 4.0f;
            const ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
            const float indentOffset = (depth + 1) * ImGui::GetStyle().IndentSpacing;

            const ImRect dropRect(
                cursorScreenPos.x + indentOffset,
                cursorScreenPos.y,
                cursorScreenPos.x + ImGui::GetContentRegionAvail().x,
                cursorScreenPos.y + dropZoneHeight
            );

            const float lineY = dropRect.Min.y + dropZoneHeight * 0.5f;

            // Create unique ID
            size_t parentHash = std::hash<TNodeId>{}(parentNode);
            size_t targetHash = std::hash<TNodeId>{}(targetBeforeNode);
            const ImGuiID dropTargetId = ImGui::GetID(
                reinterpret_cast<void*>(parentHash ^ (targetHash << 16) ^ 0xDEADBEEFull)
            );

            if (ImGui::BeginDragDropTargetCustom(dropRect, dropTargetId)) {
                const ImGuiPayload* payload = ImGui::GetDragDropPayload();
                if (payload && payload->IsDataType(m_dragDropPayloadName.c_str())) {
                    TNodeId draggedNode = *static_cast<const TNodeId*>(payload->Data);

                    bool isValidDrop = draggedNode != targetBeforeNode &&
                                      draggedNode != parentNode &&
                                      !isDescendantOf(draggedNode, parentNode);

                    if (isValidDrop) {
                        const ImU32 feedbackColor = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
                        ImGui::GetWindowDrawList()->AddLine(
                            {dropRect.Min.x, lineY},
                            {dropRect.Max.x, lineY},
                            feedbackColor,
                            2.5f
                        );

                        if (ImGui::AcceptDragDropPayload(m_dragDropPayloadName.c_str(), ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                            reparentNode(draggedNode, parentNode, targetBeforeNode);
                            markFullRebuildNeeded();
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::Dummy({0, dropZoneHeight});
        }

        void handleEmptySpaceClick() {
            if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
                clearSelection();
                onSelectionChanged(getInvalidNodeId());
            }
        }

        void handleEmptySpaceContextMenu() {
            if (!m_isNodeHovered && ImGui::BeginPopupContextWindow("##EmptySpaceMenu",
                    ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
                drawEmptySpaceContextMenu();
                ImGui::EndPopup();
            }
        }

        void updateDirtyNodes() {
            if (m_dirtyNodes.empty()) {
                rebuildCache();
                return;
            }

            for (TNodeId nodeId : m_dirtyNodes) {
                if (!isValidNodeId(nodeId)) {
                    m_nodeCache.erase(nodeId);
                    continue;
                }

                auto& cache = getOrCreateCache(nodeId);
                cache.name = getNodeName(nodeId);

                TNodeId newParent = getNodeParent(nodeId);
                if (cache.parent != newParent) {
                    // Remove from old parent
                    if (isValidNodeId(cache.parent)) {
                        auto oldParentIt = m_nodeCache.find(cache.parent);
                        if (oldParentIt != m_nodeCache.end()) {
                            auto& oldChildren = oldParentIt->second.children;
                            oldChildren.erase(
                                std::remove(oldChildren.begin(), oldChildren.end(), nodeId),
                                oldChildren.end()
                            );
                        }
                    }

                    // Add to new parent
                    cache.parent = newParent;
                    if (isValidNodeId(newParent)) {
                        auto newParentIt = m_nodeCache.find(newParent);
                        if (newParentIt != m_nodeCache.end()) {
                            newParentIt->second.children.push_back(nodeId);
                        }
                    }
                }
            }

            m_dirtyNodes.clear();
        }

        NodeCache& getOrCreateCache(TNodeId nodeId) {
            auto it = m_nodeCache.find(nodeId);
            if (it != m_nodeCache.end()) {
                return it->second;
            }

            NodeCache cache;
            cache.id = nodeId;
            cache.name = getNodeName(nodeId);
            cache.parent = getNodeParent(nodeId);
            cache.isLoaded = true;

            auto [insertIt, _] = m_nodeCache.emplace(nodeId, std::move(cache));
            return insertIt->second;
        }

        void markNodeDirty(TNodeId nodeId) {
            m_dirtyNodes.insert(nodeId);
            m_isDirty = true;
        }

        void invalidateSubtree(TNodeId nodeId) {
            m_dirtyNodes.insert(nodeId);

            auto it = m_nodeCache.find(nodeId);
            if (it != m_nodeCache.end()) {
                for (TNodeId child : it->second.children) {
                    invalidateSubtree(child);
                }
            }

            m_isDirty = true;
        }
    };

}
