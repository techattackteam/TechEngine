#pragma once
#include <string>
#include <vector>

#include "Widget.hpp"

namespace TechEngine {
    class WidgetsRegistry {
    public:
        bool loadJson(const std::string& jsonFilePath, bool base);

        std::shared_ptr<Widget> createBaseWidget(const std::string& name);

        std::shared_ptr<Widget> createWidget(const std::string& name);

        const std::vector<Widget>& getWidgets() const {
            return m_widgetsTemplates;
        }

        const std::vector<Widget> getBaseWidgets() const;

    private:
        std::vector<Widget> m_baseWidgetsTemplates;
        std::vector<Widget> m_widgetsTemplates;
    };
}
