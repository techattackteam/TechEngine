#pragma once

#include "../core/Window.hpp"
#include "../event/EventDispatcher.hpp"

namespace Engine {
    class Panel {
    protected:
        Window window;

        bool mainPanel = false;
    public:
        explicit Panel(const std::string &name = "Panel", bool mainPanel = false);

        virtual ~Panel();

        virtual void onUpdate() = 0;

        bool isMainPanel() const;

    };


}

