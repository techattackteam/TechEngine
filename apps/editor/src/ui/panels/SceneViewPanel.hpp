#pragma once

#include <ui/panels/ViewPanel.hpp>

namespace TechEngine {
    class SceneViewPanel final : public ViewPanel {
    public:
        SceneViewPanel();

    protected:
        void describeCamera(ViewRequest& request) const override;
    };
}
