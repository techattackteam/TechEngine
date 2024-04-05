#pragma once

#include "eventSystem/EventDispatcher.hpp"

namespace TechEngine {

    void dispatchEvent(TechEngine::Event *event);

    void subscribeEvent(const EventType &type, const std::function<void(TechEngine::Event *)> &callback);
}


