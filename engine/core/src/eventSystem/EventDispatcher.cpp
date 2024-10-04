#include "EventDispatcher.hpp"

#include "core/Logger.hpp"

namespace TechEngine {
    EventDispatcher::EventDispatcher() {
        TE_LOGGER_INFO("EventDispatcher created");
    }

    EventDispatcher::EventDispatcher(const EventDispatcher& other) {
        m_eventManager = other.m_eventManager;
        TE_LOGGER_INFO("EventDispatcher copied");
    }

    EventDispatcher& EventDispatcher::operator=(const EventDispatcher& rhs) {
        if (this == &rhs) {
            return *this;
        }
        TE_LOGGER_INFO("EventDispatcher copied");
        return *this;
    }

    std::shared_ptr<System> EventDispatcher::clone() {
        return std::make_shared<EventDispatcher>(*this);
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
