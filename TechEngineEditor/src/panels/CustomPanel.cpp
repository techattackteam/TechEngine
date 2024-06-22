#include "CustomPanel.hpp"
#include "events/panels/RegisterCustomPanel.hpp"

namespace TechEngine {
    CustomPanel::CustomPanel(const std::string& name) : Panel(name) {
    }

    CustomPanel::~CustomPanel() = default;

    void CustomPanel::init() {
    }


    void CustomPanel::setupImGuiContext(ImGuiContext* imGuiContext) {
        this->imGuiContext = imGuiContext;
    }
}
