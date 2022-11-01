#pragma once

#include "event/EventDispatcher.hpp"

namespace TechEngine {
    class Panel {
    protected:
        std::string name;
    public:
        explicit Panel(const std::string &name = "Panel");

        virtual ~Panel();

        virtual void onUpdate() = 0;
    };
}

