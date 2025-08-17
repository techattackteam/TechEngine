#pragma once

#include "Panel.hpp"
#include "ui/Widget.hpp"

namespace TechEngine {
    class UIEditor;

    class UIHierarchyPanel : public Panel {
    private:
        SystemsRegistry& m_appSystemsRegistry;
        UIEditor* m_editor = nullptr;
        std::vector<std::shared_ptr<Widget>> m_widgetsToDelete;
        std::vector<std::shared_ptr<Widget>> m_widgetsToCreate;
        std::vector<std::shared_ptr<Widget>> m_widgetsOrder;
        bool m_isWidgetHovered = false;

    public:
        UIHierarchyPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry);

        void onInit() override;

        void onUpdate() override;

        void setEditor(UIEditor* editor);

    private:
        void moveWidget(const std::shared_ptr<Widget>& childWidget, const std::shared_ptr<Widget>& newParent, int newIndex);

        void drawWidgetNode(const std::shared_ptr<Widget>& widget);
    };
}
