#pragma once

#include <string>

namespace Engine {
    using ComponentName = std::string;

    class Component {
        ComponentName name;
    public:
        virtual void getInfo() = 0;

        ComponentName &getName();
    };
}


