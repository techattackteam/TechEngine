#include "RenderPassHierarchyPanel.hpp"

#include "core/Logger.hpp"

namespace TechEngine {

    RenderPassHierarchyPanel::RenderPassHierarchyPanel(
        SystemsRegistry& editorSystemsRegistry,
        HierarchyNode& selectedNode
    ) : HierarchyPanelBase<RenderPassNode>(editorSystemsRegistry, HierarchyPanelConfig{
            .allowReparenting = false,  // Render passes can't be reparented
            .allowReordering = false,   // Can't reorder
            .allowCreation = false,     // Can't create new passes from UI
            .allowDeletion = false,     // Can't delete passes from UI
            .allowRenaming = false,     // Can't rename passes
            .showDropZones = false,     // No drag-drop
            .preserveWorldTransform = false
        }),
        m_legacySelectedNode(selectedNode)
    {
        m_dragDropPayloadName = "RENDER_PASS_DRAG";  // Not used since dragging is disabled
        initializeRenderPasses();
    }

    void RenderPassHierarchyPanel::initializeRenderPasses() {
        m_renderPasses = {
            {1, "Light Culling"},
            {2, "Ambient Occlusion Settings"},
            {3, "Bloom Settings"},
            {4, "Chromatic Aberration Settings"},
            {5, "Vignette Settings"},
            {6, "Gamma Settings"},
            {7, "Color Grading Settings"},
            {8, "Film Grain Settings"},
            {9, "Fog Settings"},
            {10, "Volumetric Settings"}
        };
    }

    void RenderPassHierarchyPanel::onHierarchyInit() {
        // Static list, no events to subscribe to
    }

    void RenderPassHierarchyPanel::rebuildCache() {
        m_nodeCache.clear();
        m_rootNodes.clear();

        for (const auto& pass : m_renderPasses) {
            NodeCache cache;
            cache.id = pass;
            cache.parent = getInvalidNodeId();
            cache.name = pass.name;
            cache.depth = 0;
            cache.isLoaded = true;
            // Render passes have no children

            m_nodeCache[pass] = std::move(cache);
            m_rootNodes.push_back(pass);
        }

        m_totalNodeCount = m_renderPasses.size();
    }

    std::string RenderPassHierarchyPanel::getNodeName(RenderPassNode node) const {
        return node.name;
    }

    RenderPassNode RenderPassHierarchyPanel::getNodeParent(RenderPassNode node) const {
        return getInvalidNodeId();
    }

    std::vector<RenderPassNode> RenderPassHierarchyPanel::getNodeChildren(RenderPassNode node) const {
        return {};
    }

    void RenderPassHierarchyPanel::onSelectionChanged(RenderPassNode newSelection) {
        if (isValidNodeId(newSelection)) {
            HierarchyNode node;
            node.type = HierarchyNode::NodeType::RenderPass;
            node.id = newSelection.id;
            node.name = newSelection.name;
            node.depth = 0;
            node.isOpen = true;
            node.renderPassName = new std::string(newSelection.name);
            m_legacySelectedNode = node;

            TE_LOGGER_INFO("Render Pass '{}' selected", newSelection.name);
        } else {
            m_legacySelectedNode = HierarchyNode();
        }
    }

    void RenderPassHierarchyPanel::drawEmptySpaceContextMenu() {
        ImGui::TextDisabled("Render passes are read-only");
    }

    void RenderPassHierarchyPanel::drawNodeContextMenu(RenderPassNode node) {
        ImGui::TextDisabled("Render Pass: %s", node.name.c_str());
        ImGui::Separator();

        if (ImGui::MenuItem("View Settings")) {
            TE_LOGGER_INFO("View settings for render pass: {}", node.name);
        }
    }

}
