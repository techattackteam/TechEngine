#pragma once
#include <string>
#include <vector>

#include "Widget.hpp"

namespace TechEngine {
    class WidgetsRegistry {
    public:
        bool load(const std::string& jsonFilePath);

        const std::vector<Widget>& getWidgets() const {
            return m_widgets;
        }

        Widget* createWidget(const std::string& name);

    private:
        std::vector<Widget> m_widgets;
    };
}
