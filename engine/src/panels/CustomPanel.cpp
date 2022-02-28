#include "CustomPanel.hpp"
#include "../event/events/panels/RegisterCustomPanel.hpp"

namespace Engine {
    CustomPanel::CustomPanel(const std::string &name) : Panel(name, false) {
        EventDispatcher::getInstance().dispatch(new RegisterCustomPanel(this));
    }

    CustomPanel::~CustomPanel() = default;


    void CustomPanel::setupImGuiContext(ImGuiContext *imGuiContext) {
        this->imGuiContext = imGuiContext;
    }
}


