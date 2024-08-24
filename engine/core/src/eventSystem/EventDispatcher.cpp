#include "EventDispatcher.hpp"

#include "core/Logger.hpp"

namespace TechEngine {
    EventDispatcher::EventDispatcher() {
    }

    EventDispatcher::EventDispatcher(const EventDispatcher& other) {
        m_eventManager = other.m_eventManager;
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
}
