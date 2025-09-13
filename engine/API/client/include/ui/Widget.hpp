#pragma once


#include <memory>
#include <string>

#include "common/include/core/ExportDLL.hpp"

namespace TechEngine {
    class Widget;
}

namespace TechEngineAPI {
    class API_DLL Widget {
    private:
        TechEngine::Widget* m_widget = nullptr;

    public:
        explicit Widget(std::shared_ptr<TechEngine::Widget>& widget);

        const std::string& getName();
    };
}
