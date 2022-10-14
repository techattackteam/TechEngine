#pragma once

#include "ScriptMacro.hpp"

namespace TechEngine {
    class Script {
    public:
        Script();

        virtual ~Script();

        virtual void onStart() = 0;

        virtual void onFixedUpdate() = 0;

        virtual void onUpdate() = 0;

        virtual void onClose();
    };
}

