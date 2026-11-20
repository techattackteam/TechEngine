#include <ui/panels/GameViewPanel.hpp>

#include <cstdint>

namespace TechEngine {
    static constexpr std::uint64_t GAME_VIEW_TOKEN = VIEW_TEXTURE_TOKEN_BIT | 2;

    GameViewPanel::GameViewPanel() : ViewPanel("Game", GAME_VIEW_TOKEN) {
    }

    void GameViewPanel::describeCamera(ViewRequest& request) const {
        request.cameraSource = ViewCameraSource::MainCamera;
    }
}
