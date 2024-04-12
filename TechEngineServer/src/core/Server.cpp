#include "Server.hpp"
#include "external/EntryPoint.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    Server::Server() : AppCore() {
        timer.init();
        TE_LOGGER_INFO("Server started!");
    }

    Server::~Server() {
    }

    void Server::run() {
        while (running) {
            timer.addAccumulator(timer.getDeltaTime());
            while (timer.getAccumulator() >= timer.getTPS()) {
                timer.updateTicks();

                eventDispatcher.fixedSyncEventManager.execute();
                eventDispatcher.syncEventManager.execute();
                ScriptEngine::getInstance()->onFixedUpdate();
                ScriptEngine::getInstance()->onUpdate();
                sceneManager.getScene().fixedUpdate();
                sceneManager.getScene().update();
                onFixedUpdate();
                timer.addAccumulator(-timer.getTPS());
            }
            timer.updateInterpolation();
            timer.update();
            timer.updateFPS();
        }
    }

    void Server::onUpdate() {
    }

    void Server::onFixedUpdate() {
    }
}

TechEngine::AppCore* TechEngine::createApp() {
    return new TechEngine::Server();
}
