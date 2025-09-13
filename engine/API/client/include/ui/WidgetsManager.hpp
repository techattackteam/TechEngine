#pragma once

#include "common/include/core/ExportDLL.hpp"

#include <memory>
#include <unordered_map>

#include "eventSystem/EventDispatcher.hpp"

namespace TechEngine {
    class Widget;
    class WidgetsRegistry;
}

namespace TechEngineAPI {
    class Widget;

    class API_DLL WidgetsManager {
    private:
        friend class ClientEntry;

        inline static std::unordered_map<TechEngine::Widget*, std::shared_ptr<Widget>> widgets;

        static void init(TechEngine::WidgetsRegistry* widgetsRegistry, TechEngine::EventDispatcher* dispatcher);

        static void shutdown();
    };
}
