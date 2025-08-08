#include "Panel.hpp"

#include "events/input/KeyPressedEvent.hpp"
#include "events/input/KeyReleasedEvent.hpp"
#include "events/input/MouseMoveEvent.hpp"
#include "events/input/MouseScrollEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    Panel::Panel(SystemsRegistry& editorSystemsRegistry) : m_editorSystemsRegistry(editorSystemsRegistry) {
    }

    void Panel::init(const std::string& name, ImGuiWindowClass* parentDockSpaceClass, bool isVisible) {
        assert((!name.empty() && "Panel name is not set") || (parentDockSpaceClass != nullptr && "Parent dock space class is not set"));

        this->m_name = name;
        this->m_parentDockSpaceClass = parentDockSpaceClass;
        this->m_isVisible = isVisible;
        m_windowFlags |= ImGuiWindowFlags_NoCollapse;

        m_editorSystemsRegistry.getSystem<EventDispatcher>().subscribe<MouseScrollEvent>([this](const std::shared_ptr<Event>& event) {
            this->onMouseScrollEvent(((MouseScrollEvent*)event.get())->getXOffset(), ((MouseScrollEvent*)event.get())->getYOffset());
        });

        m_editorSystemsRegistry.getSystem<EventDispatcher>().subscribe<MouseMoveEvent>([this](const std::shared_ptr<Event> event) {
            this->onMouseMoveEvent(((MouseMoveEvent*)event.get())->getDelta());
        });

        m_editorSystemsRegistry.getSystem<EventDispatcher>().subscribe<KeyPressedEvent>([this](const std::shared_ptr<Event> event) {
            this->onKeyPressedEvent(((KeyPressedEvent*)event.get())->getKey());
        });

        m_editorSystemsRegistry.getSystem<EventDispatcher>().subscribe<KeyReleasedEvent>([this](const std::shared_ptr<Event> event) {
            this->onKeyReleasedEvent(((KeyReleasedEvent*)event.get())->getKey());
        });
        id = nextID++;

        onInit();
    }

    void Panel::update() {
        assert((!this->m_name.empty() && "Panel name is not set") || (this->m_parentDockSpaceClass != nullptr && "Parent dock space class is not set"));
        ImGui::SetNextWindowClass(m_parentDockSpaceClass);
        if (!m_styleVars.empty()) {
            for (const auto& [styleVar, value]: m_styleVars) {
                if (std::holds_alternative<ImVec2>(value)) {
                    ImVec2 vector = std::get<ImVec2>(value);
                    ImGui::PushStyleVar(styleVar, vector);
                } else if (std::holds_alternative<float>(value)) {
                    float f = std::get<float>(value);
                    ImGui::PushStyleVar(styleVar, f);
                }
            }
        }

        ImGui::Begin((this->m_name + "##" + std::to_string(id)).c_str(), nullptr, m_windowFlags);
        if (!m_styleVars.empty()) {
            ImGui::PopStyleVar(m_styleVars.size());
        }

        this->onUpdate();
        shortcuts();
        ImGui::End();
    }

    void Panel::onKeyPressedEvent(Key& key) {
        m_keysPressed.push_back(key);
    }

    void Panel::onKeyReleasedEvent(Key& key) {
    }

    void Panel::onMouseScrollEvent(float xOffset, float yOffset) {
    }

    void Panel::onMouseMoveEvent(glm::vec2 delta) {
    }


    void Panel::shortcuts() {
        if (ImGui::IsWindowFocused()) {
            processShortcuts();
        }
        m_keysPressed.clear();
    }

    void Panel::processShortcuts() {
    }
}
