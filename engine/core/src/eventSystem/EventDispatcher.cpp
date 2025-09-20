#include "EventDispatcher.hpp"

#include "core/Logger.hpp"

namespace TechEngine {
    void EventDispatcher::init() {
    }


    void EventDispatcher::onUpdate() {
        m_eventManager.execute();
    }

    void EventDispatcher::onFixedUpdate() {
        System::onFixedUpdate();
    }

    void EventDispatcher::shutdown() {
        m_eventManager.shutdown();
    }

    void EventDispatcher::registerEditorWatchDog(const std::function<void(std::shared_ptr<Event>)>& editorWatchDogCallback) {
        m_eventManager.registerEditorWatchDog(editorWatchDogCallback);
    }

    void EventDispatcher::executeSingleEvent(const std::shared_ptr<Event>& event) {
        m_eventManager.executeSingleEvent(event);
    }
}
