#include "EventDispatcher.hpp"

namespace TechEngine {
    EventDispatcher::EventDispatcher() {
    }

    void EventDispatcher::init() {
    }


    void EventDispatcher::onUpdate() {
        m_eventManager.execute();
    }

    void EventDispatcher::onFixedUpdate() {
        System::onFixedUpdate();
    }

    void EventDispatcher::shutdown() {
    }

    void EventDispatcher::dispatch(Event* event) {
        m_eventManager.dispatch(event);
    }
}
