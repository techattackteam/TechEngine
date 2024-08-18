#include "Script.hpp"

#include "ScriptRegister.hpp"

namespace TechEngine {
    Script::Script() {
        registerScript(this);
    }

    Script::~Script() = default;


    /*void Script::onStart() {
        onStartFunc();
    }*/

    void Script::onFixedUpdate() {
    }

    void Script::onUpdate() {
    }

    void Script::onClose() {
    }

    const std::string& Script::getName() const {
        return name;
    }

    Script* Script::registerScript(Script* script) {
        ScriptRegister::registerScript(script);
        return script;
    }
}
