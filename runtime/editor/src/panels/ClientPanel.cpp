#include "ClientPanel.hpp"
#include "LoggerPanel.hpp"
#include "events/input/KeyHoldEvent.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "events/input/KeyReleasedEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"

namespace TechEngine {
    ClientPanel::ClientPanel(SystemsRegistry& editorSystemsRegistry,
                             SystemsRegistry& clientSystemsRegistry,
                             LoggerPanel& loggerPanel) : RuntimePanel(editorSystemsRegistry,
                                                                      clientSystemsRegistry,
                                                                      loggerPanel) {
        m_projectType = ProjectType::Client;

        //"Transfer" event from editor to client
        editorSystemsRegistry.getSystem<EventDispatcher>().subscribe<KeyPressedEvent>([this](const std::shared_ptr<Event>& event) {
            m_appSystemsRegistry.getSystem<EventDispatcher>().dispatch<KeyPressedEvent>(Key(dynamic_cast<KeyPressedEvent*>(event.get())->getKey()));
        });
        editorSystemsRegistry.getSystem<EventDispatcher>().subscribe<KeyHoldEvent>([this](const std::shared_ptr<Event>& event) {
            m_appSystemsRegistry.getSystem<EventDispatcher>().dispatch<KeyHoldEvent>(Key(dynamic_cast<KeyHoldEvent*>(event.get())->getKey()));
        });
        editorSystemsRegistry.getSystem<EventDispatcher>().subscribe<KeyReleasedEvent>([this](const std::shared_ptr<Event>& event) {
            m_appSystemsRegistry.getSystem<EventDispatcher>().dispatch<KeyReleasedEvent>(Key(dynamic_cast<KeyReleasedEvent*>(event.get())->getKey()));
        });
    }
}
