#include "Wrapper.hpp"


namespace TechEngine {
    void dispatchEvent(TechEngine::Event *event) {
        TechEngine::EventDispatcher::getInstance().dispatch(event);
    }

    void subscribeEvent(const EventType &type, const std::function<void(TechEngine::Event * )> &callback) {
        TechEngine::EventDispatcher::getInstance().subscribe(type, callback);
    }
}