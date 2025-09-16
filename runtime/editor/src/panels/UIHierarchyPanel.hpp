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

        SystemsRegistry& m_appSystemsRegistry;
        UIEditor* m_editor = nullptr;

        std::vector<HierarchyNode> m_displayList;


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

        void drawDropZone(const std::shared_ptr<Widget>& parent, const std::shared_ptr<Widget>& targetBefore);

        void drawWidgetNode(HierarchyNode& node);

        void openCreateWidgetMenu(const std::string& title, const std::shared_ptr<Widget>& parent = nullptr);
    };
}
