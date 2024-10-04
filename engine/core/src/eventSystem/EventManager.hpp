#pragma once

#include "events/Event.hpp"

#include <functional>
#include <memory>
#include <queue>
#include <typeindex>
#include <unordered_map>

namespace TechEngine {
    using Observer = std::function<void(std::shared_ptr<Event>)>;
    using ObserversVector = std::vector<std::shared_ptr<Observer>>;

    class CORE_DLL EventManager {
    private:
        std::queue<std::shared_ptr<Event>> m_dispatchedEvents;
        std::unordered_map<std::type_index, ObserversVector> m_observers;

    public:
        EventManager() = default;

        EventManager(const EventManager& rhs);

        EventManager& operator=(const EventManager& rhs);

        template<typename EventType>
        void subscribe(const Observer& callback) {
            static_assert(std::is_base_of<Event, EventType>::value, "T must derive from Event");
            std::type_index type = typeid(EventType);
            if (m_observers.count(type) == 0) {
                m_observers[typeid(EventType)] = ObserversVector();
            }
            m_observers[type].emplace_back(std::make_shared<Observer>(callback));
        }

        template<typename EventType>
        void unsubscribe(const EventType& type, const Observer& callback) {
            static_assert(std::is_base_of<Event, EventType>::value, "T must derive from Event");
            auto& callbacks = m_observers[typeid(type)];
            callbacks.erase(std::remove(callbacks.begin(), callbacks.end(), callback), callbacks.end());
        }

        void dispatch(const std::shared_ptr<Event>& event);

        void execute();
    };
}
