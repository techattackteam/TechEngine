#pragma once

#include "HierarchyPanelBase.hpp"
#include "HierarchyNode.hpp"

#include <string>

namespace TechEngine {

    /**
     * @brief Simple data structure for render pass nodes
     */
    struct RenderPassNode {
        int id = -1;
        std::string name;

        bool operator==(const RenderPassNode& other) const {
            return id == other.id;
        }
    };

}

// Hash specialization for RenderPassNode
namespace std {
    template<>
    struct hash<TechEngine::RenderPassNode> {
        size_t operator()(const TechEngine::RenderPassNode& node) const {
            return std::hash<int>{}(node.id);
        }
    };
}

namespace TechEngine {

    /**
     * @brief Panel for displaying render passes/settings
     * This panel is read-only - no reparenting, creation, or deletion allowed
     */
    class RenderPassHierarchyPanel : public HierarchyPanelBase<RenderPassNode> {
    private:
        // For compatibility with existing selection system
        HierarchyNode& m_legacySelectedNode;

        // Static list of render passes
        std::vector<RenderPassNode> m_renderPasses;

    public:
        RenderPassHierarchyPanel(
            SystemsRegistry& editorSystemsRegistry,
            HierarchyNode& selectedNode
        );

        ~RenderPassHierarchyPanel() override = default;

    protected:
        // === HierarchyPanelBase interface ===

        RenderPassNode getInvalidNodeId() const override {
            return RenderPassNode{-1, ""};
        }

        bool isValidNodeId(RenderPassNode id) const override {
            return id.id != -1;
        }

        void rebuildCache() override;

        std::string getNodeName(RenderPassNode node) const override;

        RenderPassNode getNodeParent(RenderPassNode node) const override;

        std::vector<RenderPassNode> getNodeChildren(RenderPassNode node) const override;

        void onSelectionChanged(RenderPassNode newSelection) override;

        void onHierarchyInit() override;

        void drawEmptySpaceContextMenu() override;

        void drawNodeContextMenu(RenderPassNode node) override;

    private:
        void initializeRenderPasses();
    };

}
