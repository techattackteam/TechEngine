#pragma once

#include "../core/Window.hpp"
#include "../event/EventDispatcher.hpp"

namespace TechEngine {
    class Panel {
    protected:
        bool mainPanel = false;
        std::string name;
    public:
        explicit Panel(const std::string &name = "Panel", bool mainPanel = false);

        virtual ~Panel();

        virtual void onUpdate() = 0;

        bool isMainPanel() const;

    };


}

