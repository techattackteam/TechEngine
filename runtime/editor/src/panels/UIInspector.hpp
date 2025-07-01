#pragma once
#include "Panel.hpp"
#include "../ui/Widget.hpp"


namespace TechEngine {
    class UIInspector : public Panel {
    private:
        Widget* m_selectedWidget = nullptr;

    public:
        explicit UIInspector(SystemsRegistry& editorSystemsRegistry);

        void onInit() override;

        void onUpdate() override;

        void setSelectedWidget(Widget* widget);
    };
}
