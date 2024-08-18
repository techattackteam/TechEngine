#include "Script.hpp"
#include "script/Script.hpp"

namespace TechEngineAPI {
    Script::Script() {
        script = new TechEngine::Script();

        script->onStartFunc = [this]() {
            onStart();
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
