#pragma once

#include "input/Key.hpp"
#include <imgui.h>
#include <string>
#include <variant>
#include <vector>
#include <glm/vec2.hpp>

#include "core/UUID.hpp"


namespace TechEngine {
    class SystemsRegistry;
}

namespace TechEngine {
    class Panel {
    protected:
        inline static int nextID = 0;
        int id;
        bool m_isVisible = true;
        std::string m_name;
        ImGuiWindowClass* m_parentDockSpaceClass = nullptr; // from parent otherwise editor window
        ImGuiWindowFlags m_windowFlags = ImGuiWindowFlags_None;
        std::vector<std::tuple<ImGuiStyleVar, std::variant<ImVec2, float>>> m_styleVars;
        std::vector<Key> m_keysPressed;
        SystemsRegistry& m_editorSystemsRegistry;

    public:
        Panel(SystemsRegistry& m_systemsRegistry);

        virtual ~Panel() = default;

        virtual void init(const std::string& name, ImGuiWindowClass* parentDockSpaceClass, bool isVisible = true);

        virtual void update();

        virtual void onInit() = 0;

        virtual void onUpdate() = 0;

        virtual void onKeyPressedEvent(Key& key);

        virtual void onKeyReleasedEvent(Key& key);

        virtual void onMouseScrollEvent(float xOffset, float yOffset);

        virtual void onMouseMoveEvent(glm::vec2 delta);

        void shortcuts();

        virtual void processShortcuts();

        const std::string& getName() const {
            return m_name;
        }

        const int& getId() const {
            return id;
        }
    };
}
