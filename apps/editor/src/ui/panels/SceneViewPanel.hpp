#pragma once

#include <ui/panels/Panel.hpp>

namespace TechEngine {
    class SceneViewPanel final : public Panel {
    public:
        SceneViewPanel();

    protected:
        void onDraw(const EditorSnapshot& snapshot) override;
    };
}
