#pragma once

#include "TechEngine/core/core/CoreExportDLL.hpp"
#include <functional>
#include <memory>
#include <typeindex>

namespace TechEngine {
    class EventManager;
    class Event;

    class CORE_DLL EventDispatcher {
    private:
        friend class Editor;
        friend class EventManager;

        EventManager* m_eventManager = nullptr;

        EventDispatcher(EventManager* eventManager);

    public:
        template<typename EventType>
        void subscribe(const std::function<void(std::shared_ptr<Event>)>& callback) {
            static_assert(std::is_base_of<Event, EventType>::value, "T must derive from Event");
            const std::type_index type = typeid(EventType);
            subscribeImpl(type, callback);
        }

        template<typename EventType>
        void unsubscribe(const std::function<void(std::shared_ptr<Event>)>& callback) {
            static_assert(std::is_base_of<Event, EventType>::value, "T must derive from Event");
            unsubscribeImpl(typeid(EventType), callback);
        }

        template<typename EventType, typename... Args>
        void dispatch(Args... args) {
            dispatchImpl(std::make_shared<EventType>(args...));
        }

    private:
        void subscribeImpl(std::type_index typeID, const std::function<void(std::shared_ptr<Event>)>& callback);

        void unsubscribeImpl(std::type_index typeID, const std::function<void(std::shared_ptr<Event>)>& callback);

        void dispatchImpl(const std::shared_ptr<Event>& event);
    };
}
