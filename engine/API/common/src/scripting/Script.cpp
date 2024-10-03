#include "scripting/Script.hpp"
#include "script/Script.hpp"

namespace TechEngineAPI {
    Script::Script() {
        script = new TechEngine::Script();

        script->onStartFunc = [this]() {
            onStart();
        };
        script->onUpdateFunc = [this]() {
            onUpdate();
        };
        script->onFixedUpdateFunc = [this]() {
            onFixedUpdate();
        };
        script->onCloseFunc = [this]() {
            onClose();
        };
    }

    Script::~Script() {
    }

    void Script::onStart() {
    }

    void Script::onFixedUpdate() {
    }

    void Script::onUpdate() {
    }

    void Script::onClose() {
    }

    void Script::setName(const std::string& name) {
        this->name = name;
        script->name = name;
    }

    const std::string& Script::getName() const {
        return name;
    }
}
