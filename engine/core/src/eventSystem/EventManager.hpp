#pragma once

#include "TechEngine/core/events/Event.hpp"
#include "TechEngine/core/eventSystem/EventDispatcher.hpp"
#include "systems/System.hpp"

#include <functional>
#include <memory>
#include <queue>
#include <shared_mutex>
#include <typeindex>
#include <unordered_map>

namespace TechEngine {
    using Observer = std::function<void(std::shared_ptr<Event>)>;
    using ObserversVector = std::vector<std::shared_ptr<Observer>>;

    class CORE_DLL EventManager : public System {
    private:
        std::queue<std::shared_ptr<Event>> m_dispatchedEvents;
        std::unordered_map<std::type_index, ObserversVector> m_observers;
        std::function<void(std::shared_ptr<Event>)> m_editorWatchDogCallback;
        EventDispatcher m_eventDispatcher;

        mutable std::shared_mutex m_observersMutex;
        std::mutex m_queueMutex;

    public:
        EventManager();

        EventManager(const EventManager& rhs);

        EventManager& operator=(const EventManager& rhs);

        void onUpdate();

        void registerEditorWatchDog(const std::function<void(std::shared_ptr<Event>)>& editorWatchDogCallback);

        template<typename EventType>
        void subscribe(const Observer& callback) {
            static_assert(std::is_base_of<Event, EventType>::value, "T must derive from Event");
            std::type_index type = typeid(EventType);
            if (m_observers.count(type) == 0) {
                m_observers[typeid(EventType)] = ObserversVector();
            }
            m_observers[type].emplace_back(std::make_shared<Observer>(callback));
        }

        void subscribe(const std::type_index& type, const Observer& callback);

        template<typename EventType>
        void unsubscribe(const EventType& type, const Observer& callback) {
            static_assert(std::is_base_of<Event, EventType>::value, "T must derive from Event");
            auto& callbacks = m_observers[typeid(type)];
            callbacks.erase(std::remove(callbacks.begin(), callbacks.end(), callback), callbacks.end());
        }

        void unsubscribe(const std::type_index& type, const Observer& callback);

        template<typename EventType, typename... Args>
        void dispatch(Args... args) {
            static_assert(std::is_base_of<Event, EventType>::value, "T must derive from Event");
            dispatch(std::make_shared<EventType>(args...));
        }

        void dispatch(const std::shared_ptr<Event>& event);

        void execute();

        void shutdown();

        EventDispatcher& getEventDispatcher();
    };
}
