#pragma once
#include <yaml-cpp/emitter.h>

#include "core/ExportDLL.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    class CLIENT_DLL WidgetsSerializer {
    public:
        void init(SystemsRegistry& systemsRegistry);

        void serializeUI(const std::string& sceneName, SystemsRegistry& systemsRegistry);

        void deserializeUI(const std::string& sceneName, SystemsRegistry& systemsRegistry);
    };
}
