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
        std::string m_name;

        bool m_isVisible = true;
        bool m_isPanelHovered = false;
        bool m_isDragging = false;
        bool m_isResizing = false;
        bool m_isMoving = false;

        ImVec2 m_lastSize;
        ImVec2 m_lastPosition;

        ImGuiWindowClass* m_parentDockSpaceClass = nullptr; // from parent otherwise editor window
        ImGuiWindowFlags m_windowFlags = ImGuiWindowFlags_None;
        std::vector<std::tuple<ImGuiStyleVar, std::variant<ImVec2, float>>> m_styleVars;

        SystemsRegistry& m_editorSystemsRegistry;

    public:
        Panel(SystemsRegistry& editorSystemsRegistry);

        virtual ~Panel() = default;

        virtual void init(const std::string& name, ImGuiWindowClass* parentDockSpaceClass, bool isVisible = true);

        void registerShortcuts();

        virtual void update();

        virtual void onInit() = 0;

        virtual void onUpdate() = 0;

        void processInput();

        virtual void processShortcuts();

        virtual void processMouseDragging(glm::vec2 delta, unsigned long long mouseButtons);

        virtual void processMouseMoving(glm::vec2 delta);

        virtual void processMouseScroll(float yOffset);

        const std::string& getName() const {
            return m_name;
        }

        const int& getId() const {
            return id;
        }
    };
}
