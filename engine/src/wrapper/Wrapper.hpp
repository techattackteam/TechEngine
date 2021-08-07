#pragma once


#include "../engine/event/events/Event.hpp"

namespace Engine {

    void dispatchEvent(Event *event);

    void subscribeEvent(const EventType &type, const std::function<void(Event *)> &callback);
}


