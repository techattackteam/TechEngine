#include "eventSystem/EventManager.hpp"

#include "common/include/eventSystem/EventSystem.hpp"

namespace TechEngineAPI {
    void EventSystem::init(TechEngine::EventDispatcher* dispatcher) {
        m_dispatcher = dispatcher;
    }

    void EventSystem::subscribe(const std::type_index& type, const std::function<void(std::shared_ptr<Event>)>& callback) {
        if (m_externalObservers.count(type) == 0) {
            m_externalObservers[type] = std::vector<std::shared_ptr<std::function<void(std::shared_ptr<Event>)>>>();
        }
        m_externalObservers[type].push_back(std::make_shared<std::function<void(std::shared_ptr<Event>)>>(callback));
    }

    void EventSystem::dispatch(const std::shared_ptr<Event>& event) {
        m_dispatchedExternalEvents.push(event);
    }

    void EventSystem::execute() {
        uint32_t size = m_dispatchedExternalEvents.size();
        for (uint32_t i = 0; i < size; i++) {
            std::shared_ptr<Event> event = m_dispatchedExternalEvents.front();
            if (m_externalObservers.count(typeid(*event)) != 0) {
                std::vector<std::shared_ptr<std::function<void(std::shared_ptr<Event>)>>> callbacks = m_externalObservers.at(typeid(*event));
                if (!callbacks.empty()) {
                    for (auto& callback: callbacks) {
                        callback->operator()(event);
                    }
                }
            }
            m_dispatchedExternalEvents.pop();
        }
    }
}
