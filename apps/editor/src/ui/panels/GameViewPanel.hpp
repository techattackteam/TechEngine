#pragma once

#include <ui/panels/ViewPanel.hpp>

namespace TechEngine {
    class GameViewPanel final : public ViewPanel {
    public:
        GameViewPanel();

    protected:
        void describeCamera(ViewRequest& request) const override;
    };
}
