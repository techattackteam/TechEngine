#pragma once
#include <RmlUi/Core/SystemInterface.h>

#include "core/Timer.hpp"

namespace TechEngine {
    class UISystemInterface : public Rml::SystemInterface {
    public:
        UISystemInterface(Timer* timer);

        double GetElapsedTime() override;

    private:
        Timer* m_timer;
    };
};
