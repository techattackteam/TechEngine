#include "AppCore.hpp"
#include "events/appManagement/AppCloseRequestEvent.hpp"

namespace TechEngine {
    AppCore::AppCore() : timer() {
        eventDispatcher.subscribe(AppCloseRequestEvent::eventType, [this](Event *event) {
            onAppCloseRequestEvent();
        });
    }

    AppCore::~AppCore() {

    }

    void AppCore::onAppCloseRequestEvent() {
        running = false;
    }
}
