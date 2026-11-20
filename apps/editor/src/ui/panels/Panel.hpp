#pragma once

#include <imgui.h>

#include <string>
#include <variant>
#include <vector>

namespace TechEngine {
    struct EditorSnapshot;

    struct PanelStyleVar {
        ImGuiStyleVar var = ImGuiStyleVar_Alpha;
        std::variant<float, ImVec2> value;
    };

    class Panel {
    private:
        // ImGui keys the saved layout by this name, so renaming a panel loses its place in the dock.
        std::string m_name;
        std::vector<PanelStyleVar> m_styleVars;

    public:
        explicit Panel(std::string name);

        virtual ~Panel() = default;

        // False when the panel is hidden, such as a tab behind another, and onDraw did not run.
        bool draw(const EditorSnapshot& snapshot);

        const std::string& getName() const;

    protected:
        // Pushed before the panel's Begin and popped before its End, so popups opened in onDraw get them too.
        void addStyleVar(ImGuiStyleVar var, float value);

        void addStyleVar(ImGuiStyleVar var, ImVec2 value);

        virtual void onDraw(const EditorSnapshot& snapshot) = 0;
    };
}
