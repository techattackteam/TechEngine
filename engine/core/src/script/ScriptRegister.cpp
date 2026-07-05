#include "ScriptRegister.hpp"

#include "core/Logger.hpp"
#include "eventSystem/EventManager.hpp"
#include "script/ScriptEngine.hpp"
#include "systems/SystemsRegistry.hpp"
#include "TechEngine/core/script/Script.hpp"
#include "resources/ResourceSystem.hpp"
#include "scene/SceneManager.hpp"

namespace TechEngine {
    ScriptRegister::ScriptRegister() {
        instance = this;
    }

    ScriptRegister::~ScriptRegister() {
        this->scriptEngine = nullptr;
        instance = nullptr;
    }

    void ScriptRegister::init(ScriptEngine* scriptEngine) {
        this->scriptEngine = scriptEngine;
    }

    ScriptRegister* ScriptRegister::getInstance() {
        if (instance == nullptr) {
            instance = new ScriptRegister();
        }
        return instance;
    }

    void ScriptRegister::registerScript(Script* script) {
        getInstance()->scriptEngine->registerScript(script);
        script->m_scene = &getInstance()->scriptEngine->getSystemsRegistry().getSystem<SceneManager>().getActiveScene();
        script->m_resourceSystem = &getInstance()->scriptEngine->getSystemsRegistry().getSystem<ResourceSystem>();
        script->m_eventDispatcher = &getInstance()->scriptEngine->getSystemsRegistry().getSystem<EventManager>().getEventDispatcher();
    }
}
