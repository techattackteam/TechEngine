#pragma once

#include "event/EventDispatcher.hpp"
#include "stateMachine/StateMachineManager.hpp"
#include "scene/SceneHelper.hpp"
#include "events/window/WindowCloseEvent.hpp"
#include "../../TechEngineCore/src/core/App.hpp"

namespace TechEngine {
    class Engine_API App : public TechEngineCore::App {
    public:
        Scene scene;

        App();

        ~App() override;

        void run() override;

        virtual void onUpdate() = 0;

        virtual void onFixedUpdate() = 0;

    private:

        void onWindowCloseEvent(TechEngine::WindowCloseEvent *event);
    };

}

