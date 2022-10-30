#include "App.hpp"
#include "event/events/appManagement/AppCloseRequestEvent.hpp"

namespace TechEngineCore {
    App::App() : timer() {
        eventDispatcher.subscribe(TechEngine::AppCloseRequestEvent::eventType, [this](Event *event) {
            onAppCloseRequestEvent();
        });
    }

    App::~App() {

    }

    void App::onAppCloseRequestEvent() {
        running = false;
    }
}
