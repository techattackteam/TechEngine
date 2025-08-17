#pragma once

#include "Widget.hpp"
#include "systems/System.hpp"

#include <string>
#include <vector>


namespace TechEngine {
    class CLIENT_DLL WidgetsRegistry : public System {
    private:
        std::vector<Widget> m_widgetsTemplates;
        std::vector<std::shared_ptr<Widget>> m_widgets;

    public:
        WidgetsRegistry();

        void init() override;

        bool loadJson(const std::string& jsonFilePath);

        std::shared_ptr<Widget> createWidget(const std::string& name);

        const std::vector<Widget>& getWidgetsTemplates() const;

        std::vector<std::shared_ptr<Widget>>& getWidgets();
    };
}
