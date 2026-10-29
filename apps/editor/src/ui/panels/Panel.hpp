#pragma once

#include <string>

namespace TechEngine {
    struct EditorSnapshot;

    class Panel {
    private:
        // ImGui keys the saved layout by this name, so renaming a panel loses its place in the dock.
        std::string m_name;

    public:
        explicit Panel(std::string name);

        virtual ~Panel() = default;

        void draw(const EditorSnapshot& snapshot);

        const std::string& getName() const;

    protected:
        virtual void onDraw(const EditorSnapshot& snapshot) = 0;
    };
}
