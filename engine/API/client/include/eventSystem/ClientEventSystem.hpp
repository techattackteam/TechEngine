#pragma once
#include "common/include/core/ExportDLL.hpp"
#include "common/include/eventSystem/EventSystem.hpp"

namespace TechEngineAPI {
    class API_DLL ClientEventSystem : public EventSystem {
    public:
        friend class ClientEntry;

    private:
        static void init(TechEngine::EventDispatcher* dispatcher);
    };
}
