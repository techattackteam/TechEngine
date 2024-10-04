#pragma once
#include "System.hpp"

namespace TechEngine {
    class CopyableSystem : public System {
    public:
        virtual std::shared_ptr<System> clone() = 0;
    };
}
