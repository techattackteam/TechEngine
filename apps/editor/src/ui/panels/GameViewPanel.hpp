#pragma once

#include <ui/panels/Panel.hpp>

namespace TechEngine {
    class GameViewPanel final : public Panel {
    public:
        GameViewPanel();

    protected:
        void onDraw(const EditorSnapshot& snapshot) override;
    };
}
