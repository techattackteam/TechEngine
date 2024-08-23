#pragma once

#include "core/CoreExportDLL.hpp"
#include <functional>
#include <string>


namespace TechEngine {
    class CORE_DLL Script {
    private:
        friend class ScriptEngine;
        std::string name;

    public:
        Script();

        virtual ~Script();

        std::function<void()> onStartFunc;
        std::function<void()> onFixedUpdateFunc;
        std::function<void()> onUpdateFunc;
        std::function<void()> onCloseFunc;


        const std::string& getName() const;

        static Script* registerScript(Script* script);
    };
}
