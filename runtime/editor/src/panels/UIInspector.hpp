#pragma once
#include "Panel.hpp"
#include "ui/Widget.hpp"


namespace TechEngine {
    class UIInspector : public Panel {
    private:
        std::shared_ptr<Widget> m_selectedWidget = nullptr;
        SystemsRegistry& m_appSystemsRegistry;

    public:
        explicit UIInspector(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry);

        void onInit() override;

        void onUpdate() override;

        void setSelectedWidget(std::shared_ptr<Widget> widget);
    };
}
