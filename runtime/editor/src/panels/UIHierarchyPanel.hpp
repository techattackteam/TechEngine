#pragma once

#include "Panel.hpp"
#include "ui/Widget.hpp"

namespace TechEngine {
    class UIEditor;

    class UIHierarchyPanel : public Panel {
    private:
        struct HierarchyNode {
            std::shared_ptr<Widget> widget;
            int depth;
            bool isOpen;

            bool operator==(const HierarchyNode& other) const {
                return widget == other.widget;
            }

            bool operator==(const std::shared_ptr<Widget>& otherWidget) const {
                return widget == otherWidget;
            }
        };

        // A command queue for deferred actions
        enum class ActionType { Move , Reparent};

        struct PendingAction {
            ActionType type;
            std::shared_ptr<Widget> subject; // The widget being moved
            std::shared_ptr<Widget> destination; // The new parent (nullptr for root)
            int index; // The new index within the destination
        };

        SystemsRegistry& m_appSystemsRegistry;
        UIEditor* m_editor = nullptr;

        std::vector<HierarchyNode> m_displayList;
        std::vector<PendingAction> m_pendingActions;

        bool m_isHierarchyDirty = true;
        bool m_isWidgetHovered = false;

    public:
        UIHierarchyPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry);

        void onInit() override;

        void onUpdate() override;

        void setEditor(UIEditor* editor);

        void setHierarchyDirty();

    private:
        void rebuildDisplayList();

        void recursiveAddToDisplayList(const std::shared_ptr<Widget>& widget, int depth);

        void applyPendingActions();

        void reparentWidget(const std::shared_ptr<Widget>& widgetToMove, const std::shared_ptr<Widget>& newParent, int newIndex);

        void drawWidgetNode(HierarchyNode& node);
    };
}
