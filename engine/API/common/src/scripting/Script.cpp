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

    const std::string& Script::getName() const {
        return name;
    }
}
