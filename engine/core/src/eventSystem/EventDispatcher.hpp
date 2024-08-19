#pragma once

#include "EventManager.hpp"
#include "systems/System.hpp"

#include <functional>

#include "core/Logger.hpp"

namespace TechEngine {
    class CORE_DLL EventDispatcher : public System {
    protected:
        bool isCopy = false;
        EventDispatcher* m_copy = nullptr;

        EventManager m_eventManager;

    public:
        EventDispatcher();

        void init() override;

        void onUpdate() override;

        void onFixedUpdate() override;

        void shutdown() override;

        template<class EventType>
        void subscribe(const std::function<void(Event*)>& callback) {
            m_eventManager.subscribe<EventType>(callback);
        }

        template<typename EventType>
        void unsubscribe(const std::function<void(Event*)>& callback) {
            m_eventManager.unsubscribe<EventType>(callback);
        }

        void dispatch(Event* event);
    };
}
