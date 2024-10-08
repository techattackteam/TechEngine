#include "ClientPanel.hpp"
#include "LoggerPanel.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"

namespace TechEngine {
    ClientPanel::ClientPanel(SystemsRegistry& editorSystemsRegistry,
                             SystemsRegistry& clientSystemsRegistry,
                             LoggerPanel& loggerPanel) : RuntimePanel(editorSystemsRegistry,
                                                                      clientSystemsRegistry,
                                                                      loggerPanel) {
        m_projectType = ProjectType::Client;

        editorSystemsRegistry.getSystem<EventDispatcher>().subscribe<KeyPressedEvent>([this](const std::shared_ptr<Event>& event) {
            m_appSystemsRegistry.getSystem<EventDispatcher>().dispatch<KeyPressedEvent>(Key(dynamic_cast<KeyPressedEvent*>(event.get())->getKey()));
        });
    }
}
