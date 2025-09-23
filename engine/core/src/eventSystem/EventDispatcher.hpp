#pragma once

#include "EventManager.hpp"
#include "systems/System.hpp"

namespace TechEngine {
    class CORE_DLL EventDispatcher : public System {
    protected:
        friend class Editor;
        EventManager m_eventManager;

    public:
        void init() override;

        void onUpdate() override;

        void onFixedUpdate() override;

        void shutdown() override;

        template<class EventType>
        void subscribe(const Observer& callback) {
            m_eventManager.subscribe<EventType>(callback);
        }

        template<typename EventType>
        void unsubscribe(const Observer& callback) {
            m_eventManager.unsubscribe<EventType>(callback);
        }

        template<typename EventType, typename... Args>
        void dispatch(Args... args) {
            m_eventManager.dispatch(std::make_shared<EventType>(args...));
        }

    private:
    };
}
