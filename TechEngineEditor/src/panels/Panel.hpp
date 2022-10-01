#pragma once

#include "event/EventDispatcher.hpp"

namespace TechEngine {
    class Panel {
    protected:
        //bool mainPanel = false;
        std::string name;
        int width = 0;
        int height = 0;
    public:
        explicit Panel(const std::string &name = "Panel");

        virtual ~Panel();

        virtual void onUpdate() = 0;

        bool isMainPanel() const;

    };


}

