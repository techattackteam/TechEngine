#pragma once

#include <system/SystemsRegistry.hpp>
#include "eventSystem/EventDispatcher.hpp"
#include "core/Key.hpp"
#include "glm/vec2.hpp"
#include <imgui.h>

namespace TechEngine {
    class Panel {
    protected:
        std::string name;
        std::vector<Key> keysPressed;
        bool m_open = true;

    public:
        explicit Panel(const std::string& name);

        virtual ~Panel();

        virtual void init() = 0;

        void update(ImGuiStyleVar ImGuiStyleVar_WindowPadding = -1, const ImVec2& valImVec2 = {0, 0}, ImGuiWindowFlags flags = 0, bool closable = false);

        virtual void onUpdate() = 0;

        virtual void onKeyPressedEvent(Key& key);

        virtual void onKeyReleasedEvent(Key& key);

        virtual void onMouseScrollEvent(float xOffset, float yOffset);

        virtual void onMouseMoveEvent(glm::vec2 delta);

        void shortcuts();

        virtual void processShortcuts();

        virtual void open();

        virtual bool isOpen();
    };
}
