#include "Wrapper.hpp"
#include "../event/EventDispatcher.hpp"

namespace TechEngine {
    void dispatchEvent(TechEngineCore::Event *event) {
        TechEngineCore::EventDispatcher::getInstance().dispatch(event);
    }

    void subscribeEvent(const EventType &type, const std::function<void(TechEngineCore::Event * )> &callback) {
        TechEngineCore::EventDispatcher::getInstance().subscribe(type, callback);
    }
}