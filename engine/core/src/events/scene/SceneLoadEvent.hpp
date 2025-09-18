#pragma once

#include <string>

#include "events/Event.hpp"

namespace TechEngine {
    class CORE_DLL SceneLoadEvent : public Event {
    private:
        std::string m_sceneName;

    public:
        SceneLoadEvent(std::string sceneName) : m_sceneName(sceneName), Event() {
        }

        const std::string& getSceneName() const {
            return m_sceneName;
        }
    };
}
