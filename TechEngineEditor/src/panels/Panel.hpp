#pragma once

#include "event/EventDispatcher.hpp"
#include "core/Key.hpp"
#include "glm/vec2.hpp"

namespace TechEngine {
    class Panel {
    protected:
        std::string name;
    public:
        explicit Panel(const std::string &name = "Panel");

        virtual ~Panel();

        virtual void onUpdate() = 0;

        virtual void onKeyPressedEvent(Key &key);

        virtual void onKeyReleasedEvent(Key &key);

        virtual void onMouseScrollEvent(float xOffset, float yOffset);

        virtual void onMouseMoveEvent(glm::vec2 delta);
    };
}

