#pragma once

#include <string>

#include "events/Event.hpp"

namespace TechEngine {
    class CORE_DLL SceneSaveEvent : public Event {
    private:
        std::string m_sceneName;

    public:
        SceneSaveEvent(std::string sceneName) : m_sceneName(sceneName), Event() {
        }

        const std::string& getSceneName() const {
            return m_sceneName;
        }
    };
}
