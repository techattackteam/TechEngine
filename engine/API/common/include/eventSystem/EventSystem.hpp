#pragma once

#include "common/include/core/ExportDLL.hpp"
#include "common/include/eventSystem//Event.hpp"

#include <functional>
#include <memory>
#include <queue>
#include <typeindex>

namespace TechEngine {
    class EventDispatcher;
}

namespace TechEngineAPI {
    class API_DLL EventSystem {
    protected:
        friend class ClientEntry;
        inline static TechEngine::EventDispatcher* m_dispatcher;

        inline static std::queue<std::shared_ptr<Event>> m_dispatchedExternalEvents;
        inline static std::unordered_map<std::type_index, std::vector<std::shared_ptr<std::function<void(std::shared_ptr<TechEngineAPI::Event>)>>>> m_externalObservers;

    public:
        static void init(TechEngine::EventDispatcher* dispatcher);

        template<class EventType>
        static void subscribe(const std::function<void(std::shared_ptr<Event>)>& callback) {
            static_assert(std::is_base_of<Event, EventType>::value || std::is_base_of<Event, EventType>::value, "T must derive from Event");
            std::type_index type = typeid(EventType);
            subscribe(type, callback);
        }

        template<class EventType, typename... Args>
        static void dispatch(Args... args) {
            dispatch(std::make_shared<EventType>(args...));
        }

        static void execute();

    private:
        static void subscribe(const std::type_index& type, const std::function<void(std::shared_ptr<Event>)>& callback);

        static void dispatch(const std::shared_ptr<Event>& event);
    };
}
