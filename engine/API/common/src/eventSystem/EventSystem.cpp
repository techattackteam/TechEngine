#include "eventSystem/EventManager.hpp"

#include "common/include/eventSystem/EventSystem.hpp"
#include "common/include/events/physics/OnCollisionEnter.hpp"
#include "common/include/events/physics/OnCollisionLeave.hpp"
#include "common/include/events/physics/OnCollisionStay.hpp"
#include "common/include/events/physics/OnTriggerEnter.hpp"
#include "common/include/events/physics/OnTriggerLeave.hpp"
#include "common/include/events/physics/OnTriggerStay.hpp"

#include "events/physics/OnCollisionEnter.hpp"
#include "events/physics/OnCollisionLeave.hpp"
#include "events/physics/OnCollisionStay.hpp"
#include "events/physics/OnTriggerEnter.hpp"
#include "events/physics/OnTriggerLeave.hpp"
#include "events/physics/OnTriggerStay.hpp"
#include "eventSystem/EventDispatcher.hpp"

namespace TechEngineAPI {
    void EventSystem::init(TechEngine::EventDispatcher* dispatcher) {
        m_dispatcher = dispatcher;
        m_dispatcher->subscribe<TechEngine::OnCollisionEnter>([](const std::shared_ptr<TechEngine::Event>& event) {
            uint32_t entity1 = dynamic_cast<TechEngine::OnCollisionEnter*>(event.get())->getEntity1();
            uint32_t entity2 = dynamic_cast<TechEngine::OnCollisionEnter*>(event.get())->getEntity2();
            dispatch<OnCollisionEnter>(entity1, entity2);
        });
        m_dispatcher->subscribe<TechEngine::OnCollisionStay>([](const std::shared_ptr<TechEngine::Event>& event) {
            uint32_t entity1 = dynamic_cast<TechEngine::OnCollisionStay*>(event.get())->getEntity1();
            uint32_t entity2 = dynamic_cast<TechEngine::OnCollisionStay*>(event.get())->getEntity2();
            dispatch<OnCollisionStay>(entity1, entity2);
        });
        m_dispatcher->subscribe<TechEngine::OnCollisionLeave>([](const std::shared_ptr<TechEngine::Event>& event) {
            uint32_t entity1 = dynamic_cast<TechEngine::OnCollisionLeave*>(event.get())->getEntity1();
            uint32_t entity2 = dynamic_cast<TechEngine::OnCollisionLeave*>(event.get())->getEntity2();
            dispatch<OnCollisionLeave>(entity1, entity2);
        });
        m_dispatcher->subscribe<TechEngine::OnTriggerEnter>([](const std::shared_ptr<TechEngine::Event>& event) {
            uint32_t entity1 = dynamic_cast<TechEngine::OnTriggerEnter*>(event.get())->getEntity1();
            uint32_t entity2 = dynamic_cast<TechEngine::OnTriggerEnter*>(event.get())->getEntity2();
            dispatch<OnTriggerEnter>(entity1, entity2);
        });
        m_dispatcher->subscribe<TechEngine::OnTriggerStay>([](const std::shared_ptr<TechEngine::Event>& event) {
            uint32_t entity1 = dynamic_cast<TechEngine::OnTriggerStay*>(event.get())->getEntity1();
            uint32_t entity2 = dynamic_cast<TechEngine::OnTriggerStay*>(event.get())->getEntity2();
            dispatch<OnTriggerStay>(entity1, entity2);
        });
        m_dispatcher->subscribe<TechEngine::OnTriggerLeave>([](const std::shared_ptr<TechEngine::Event>& event) {
            uint32_t entity1 = dynamic_cast<TechEngine::OnTriggerLeave*>(event.get())->getEntity1();
            uint32_t entity2 = dynamic_cast<TechEngine::OnTriggerLeave*>(event.get())->getEntity2();
            dispatch<OnTriggerLeave>(entity1, entity2);
        });
    }

    void EventSystem::subscribe(const std::type_index& type, const std::function<void(std::shared_ptr<Event>)>& callback) {
        if (!m_externalObservers.contains(type)) {
            m_externalObservers[type] = std::vector<std::shared_ptr<std::function<void(std::shared_ptr<Event>)>>>();
        }
        m_externalObservers[type].push_back(std::make_shared<std::function<void(std::shared_ptr<Event>)>>(callback));
    }

    void EventSystem::dispatch(const std::shared_ptr<Event>& event) {
        m_dispatchedExternalEvents.push(event);
    }

    void EventSystem::execute() {
        const uint32_t size = m_dispatchedExternalEvents.size();
        for (uint32_t i = 0; i < size; i++) {
            const std::shared_ptr<Event> event = m_dispatchedExternalEvents.front();
            if (m_externalObservers.contains(typeid(*event))) {
                std::vector<std::shared_ptr<std::function<void(std::shared_ptr<Event>)>>> callbacks = m_externalObservers.at(typeid(*event));
                if (!callbacks.empty()) {
                    for (const auto& callback: callbacks) {
                        callback->operator()(event);
                    }
                }
            }
            m_dispatchedExternalEvents.pop();
        }
    }
}
