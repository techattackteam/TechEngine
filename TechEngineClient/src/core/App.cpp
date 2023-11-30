#include "App.hpp"
#include "script/ScriptEngine.hpp"
#include "FileSystem.hpp"

namespace TechEngine {
    App::App(std::string name, int width, int height) :
            AppCore(), window(name, width, height), textureManager(), materialManager(textureManager), sceneManager(physicsEngine, materialManager), projectManager(sceneManager), physicsEngine(sceneManager.getScene()) {
        eventDispatcher.subscribe(WindowCloseEvent::eventType, [this](Event *event) {
            onWindowCloseEvent((WindowCloseEvent *) (event));
        });

        timer.init();
        textureManager.init(FileSystem::getAllFilesWithExtension(".jpg"));
        textureManager.init(FileSystem::getAllFilesWithExtension(".png"));
        materialManager.init(FileSystem::getAllFilesWithExtension(".mat"));
        sceneManager.init(FileSystem::rootPath.string());
        projectManager.init(FileSystem::rootPath);
        physicsEngine.init();
    }

    App::~App() = default;

    void App::run() {
        while (running) {
            timer.addAccumulator(timer.getDeltaTime());
            while (timer.getAccumulator() >= timer.getTPS()) {
                timer.updateTicks();

                eventDispatcher.fixedSyncEventManager.execute();
                ScriptEngine::getInstance()->onFixedUpdate();
                sceneManager.getScene().fixedUpdate();
                onFixedUpdate();
                timer.addAccumulator(-timer.getTPS());
            }

            sceneManager.getScene().update();
            timer.updateInterpolation();
            stateMachineManager.update();
            ScriptEngine::getInstance()->onUpdate();
            eventDispatcher.syncEventManager.execute();
            onUpdate();
            timer.update();
            timer.updateFPS();
        }
    }

    void App::onWindowCloseEvent(WindowCloseEvent *event) {
        running = false;
    }
}