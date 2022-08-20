#pragma once

#include <TechEngineCore.hpp>

namespace TechEngine {

    void dispatchEvent(TechEngineCore::Event *event);

    void subscribeEvent(const EventType &type, const std::function<void(TechEngineCore::Event *)> &callback);
}


