#pragma once

#include "HierarchyPanelBase.hpp"
#include "HierarchyNode.hpp"
#include "ui/Widget.hpp"

#include <memory>

namespace TechEngine {
    class WidgetsRegistry;

    /**
     * @brief Panel for displaying and managing the UI widget hierarchy
     */
    class WidgetHierarchyPanel : public HierarchyPanelBase<std::shared_ptr<Widget>> {
    public:
        using WidgetPtr = std::shared_ptr<Widget>;

    private:
        SystemsRegistry& m_appSystemsRegistry;

        // For compatibility with existing selection system
        HierarchyNode& m_legacySelectedNode;

    public:
        WidgetHierarchyPanel(
            SystemsRegistry& editorSystemsRegistry,
            SystemsRegistry& appSystemsRegistry,
            HierarchyNode& selectedNode
        );

        ~WidgetHierarchyPanel() override = default;

    protected:
        // === HierarchyPanelBase interface ===

        WidgetPtr getInvalidNodeId() const override {
            return nullptr;
        }

        bool isValidNodeId(WidgetPtr id) const override {
            return id != nullptr;
        }

        void rebuildCache() override;

        std::string getNodeName(WidgetPtr widget) const override;

        WidgetPtr getNodeParent(WidgetPtr widget) const override;

        std::vector<WidgetPtr> getNodeChildren(WidgetPtr widget) const override;

        void onSelectionChanged(WidgetPtr newSelection) override;

        void onHierarchyInit() override;

        void drawEmptySpaceContextMenu() override;

        void drawNodeContextMenu(WidgetPtr widget) override;

        void reparentNode(WidgetPtr widget, WidgetPtr newParent, WidgetPtr insertBefore) override;

        void processQueuedOperations() override;

    private:
        void rebuildCacheRecursive(WidgetPtr widget, WidgetPtr parent, int depth);

        WidgetsRegistry& getWidgetsRegistry() const;
    };

}
