#include "CustomPanel.hpp"
#include "events/panels/RegisterCustomPanel.hpp"

namespace TechEngine {
    CustomPanel::CustomPanel(const std::string& name, SystemsRegistry& editorRegistry) : Panel(name, editorRegistry) {
    }

    CustomPanel::~CustomPanel() = default;

    void CustomPanel::init() {
        Panel::init();
    }


    void CustomPanel::setupImGuiContext(ImGuiContext* imGuiContext) {
        this->imGuiContext = imGuiContext;
    }
}
