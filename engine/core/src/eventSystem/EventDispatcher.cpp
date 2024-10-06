#include "EventDispatcher.hpp"

#include "core/Logger.hpp"

namespace TechEngine {
    void EventDispatcher::init() {
    }


    void EventDispatcher::onUpdate() {
        m_eventManager.execute();
    }

    void EventDispatcher::onFixedUpdate() {
        System::onFixedUpdate();
    }

    void EventDispatcher::shutdown() {
        m_eventManager.shutdown();
    }
}
