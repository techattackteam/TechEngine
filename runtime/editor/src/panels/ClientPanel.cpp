#include "ClientPanel.hpp"
#include "LoggerPanel.hpp"
#include "events/input/KeyHoldEvent.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "events/input/KeyReleasedEvent.hpp"
#include "events/input/MouseMoveEvent.hpp"
#include "events/input/MouseScrollEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"

namespace TechEngine {
    ClientPanel::ClientPanel(SystemsRegistry& editorSystemsRegistry,
                             SystemsRegistry& clientSystemsRegistry,
                             LoggerPanel& loggerPanel) : RuntimePanel(editorSystemsRegistry,
                                                                      clientSystemsRegistry,
                                                                      loggerPanel),
                                                         m_gameView(editorSystemsRegistry, clientSystemsRegistry),
                                                         m_uiEditor(editorSystemsRegistry, clientSystemsRegistry, m_gameView),
                                                         m_profiler(editorSystemsRegistry, clientSystemsRegistry) {
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
        editorSystemsRegistry.getSystem<EventDispatcher>().subscribe<MouseMoveEvent>([this](const std::shared_ptr<Event>& event) {
            m_appSystemsRegistry.getSystem<EventDispatcher>().dispatch<MouseMoveEvent>(glm::vec2(dynamic_cast<MouseMoveEvent*>(event.get())->getFromPosition()),
                                                                                       glm::vec2(dynamic_cast<MouseMoveEvent*>(event.get())->getToPosition()));
        });
        editorSystemsRegistry.getSystem<EventDispatcher>().subscribe<MouseScrollEvent>([this](const std::shared_ptr<Event>& event) {
            m_appSystemsRegistry.getSystem<EventDispatcher>().dispatch<MouseScrollEvent>(dynamic_cast<MouseScrollEvent*>(event.get())->getXOffset(),
                                                                                         dynamic_cast<MouseScrollEvent*>(event.get())->getYOffset());
        });
    }

    void ClientPanel::onInit() {
        RuntimePanel::onInit();
        m_uiEditor.init("UI Editor", &m_dockSpaceWindowClass);
        m_gameView.init("Game View", &m_dockSpaceWindowClass);
        m_profiler.init("Profiler", &m_dockSpaceWindowClass);
    }

    void ClientPanel::onUpdate() {
        RuntimePanel::onUpdate();
        m_uiEditor.update();
        m_gameView.update();
        m_profiler.update();
    }

    void ClientPanel::setupInitialDockingLayout() {
        RuntimePanel::setupInitialDockingLayout();
        ImGui::DockBuilderDockWindow((m_gameView.getName() + "##" + std::to_string(m_gameView.getId())).c_str(),
                                     m_dockSpaceID);
        ImGui::DockBuilderDockWindow((m_uiEditor.getName() + "##" + std::to_string(m_uiEditor.getId())).c_str(),
                                     m_dockSpaceID);
        ImGui::DockBuilderDockWindow((m_profiler.getName() + "##" + std::to_string(m_profiler.getId())).c_str(),
                                     m_dockSpaceID);
    }
}
