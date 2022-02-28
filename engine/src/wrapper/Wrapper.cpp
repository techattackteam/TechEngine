#include "Wrapper.hpp"
#include "../event/EventDispatcher.hpp"

namespace Engine {
    void dispatchEvent(Event *event) {
        EventDispatcher::getInstance().dispatch(event);
    }

    void subscribeEvent(const EventType &type, const std::function<void(Event *)> &callback) {
        EventDispatcher::getInstance().subscribe(type, callback);
    }
}