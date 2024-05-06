#include "CustomPanel.hpp"
#include "events/panels/RegisterCustomPanel.hpp"

namespace TechEngine {
    CustomPanel::CustomPanel(const std::string& name, EventDispatcher& eventDispatcher) : Panel(name, eventDispatcher) {
    }

    CustomPanel::~CustomPanel() = default;


    void CustomPanel::setupImGuiContext(ImGuiContext* imGuiContext) {
        this->imGuiContext = imGuiContext;
    }
}
