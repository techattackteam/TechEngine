#include "CustomPanel.hpp"
#include "../events/panels/RegisterCustomPanel.hpp"

namespace TechEngine {
    CustomPanel::CustomPanel(const std::string &name) : Panel(name, false) {
        TechEngineCore::EventDispatcher::getInstance().dispatch(new RegisterCustomPanel(this));
    }

    CustomPanel::~CustomPanel() = default;


    void CustomPanel::setupImGuiContext(ImGuiContext *imGuiContext) {
        this->imGuiContext = imGuiContext;
    }
}


