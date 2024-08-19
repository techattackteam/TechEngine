#pragma once

#include "events/Event.hpp"

#include <functional>
#include <queue>
#include <typeindex>
#include <unordered_map>


namespace TechEngine {
    class CORE_DLL EventManager {
    private:
        std::queue<Event*> m_dispatchedEvents;
        std::unordered_map<std::type_index, std::vector<std::function<void(Event*)>>> m_observers;

    public:
        EventManager() = default;

        template<typename EventType>
        void subscribe(const std::function<void(Event*)>& callback) {
            static_assert(std::is_base_of<Event, EventType>::value, "T must derive from Event");
            std::type_index type = typeid(EventType);
            if (m_observers.count(type) == 0) {
                m_observers[typeid(EventType)] = std::vector<std::function<void(Event*)>>();
            }
            m_observers[type].emplace_back(callback);
        }

        template<typename EventType>
        void unsubscribe(const EventType& type, const std::function<void(Event*)>& callback) {
            static_assert(std::is_base_of<Event, EventType>::value, "T must derive from Event");
            auto& callbacks = m_observers[typeid(type)];
            callbacks.erase(std::remove(callbacks.begin(), callbacks.end(), callback), callbacks.end());
        }

        void dispatch(Event* event);

        void execute();
    };
}
