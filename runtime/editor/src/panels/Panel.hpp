#pragma once

#include <imgui.h>
#include <string>
#include <variant>
#include <vector>

namespace TechEngine {
    class Panel {
    protected:
        bool m_isVisible = true;
        std::string m_name;
        ImGuiWindowClass* m_parentDockSpaceClass = nullptr; // from parent otherwise editor window
        ImGuiWindowFlags m_windowFlags = ImGuiWindowFlags_None;
        std::vector<std::tuple<ImGuiStyleVar, std::variant<ImVec2, float>>> m_styleVars;

    public:
        virtual ~Panel() = default;

        virtual void init(const std::string& name, ImGuiWindowClass* parentDockSpaceClass, bool isVisible = true);

        void update();

        virtual void onInit() = 0;

        virtual void onUpdate() = 0;
    };
}
