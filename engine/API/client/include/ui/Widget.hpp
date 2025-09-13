#pragma once

#include <memory>
#include <string>
#include <vector>
#include <glm/vec2.hpp>

#include "common/include/core/ExportDLL.hpp"

namespace TechEngine {
    class Widget;
}

namespace TechEngineAPI {
    class API_DLL Widget {
    protected:
        friend class WidgetsManager;

        TechEngine::Widget* m_internalWidget = nullptr;
        inline static std::shared_ptr<Widget> m_parent;
        inline static std::vector<std::shared_ptr<Widget>> m_children;

    public:
        explicit Widget(const std::shared_ptr<TechEngine::Widget>& widget);

        virtual ~Widget() = default;

        std::shared_ptr<Widget> getParent();

        std::shared_ptr<Widget> getChild(const std::string& name);

        const std::string& getName();

        const glm::vec2& getSize();

        const void setSize(const glm::vec2& size);

        const glm::vec2& getPosition();

        const void setPosition(const glm::vec2& position);
    };
}
