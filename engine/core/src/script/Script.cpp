#include "../../include/TechEngine/core/script/Script.hpp"

#include "ScriptRegister.hpp"

namespace TechEngine {
    Script::Script() {
        registerScript(this);
    }

    Script::~Script() = default;


    void Script::onStart() {
    }

    void Script::onFixedUpdate() {
    }

    void Script::onUpdate() {
    }

    void Script::onClose() {
    }

    void Script::setName(const std::string& name) {
        this->m_name = name;
    }

    const std::string& Script::getName() const {
        return m_name;
    }

    Script* Script::registerScript(Script* script) {
        ScriptRegister::registerScript(script);
        return script;
    }
}
