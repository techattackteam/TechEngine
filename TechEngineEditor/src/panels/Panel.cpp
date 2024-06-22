#include "Panel.hpp"

#include <imgui.h>

#include "core/Logger.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "events/input/KeyReleasedEvent.hpp"
#include "events/input/MouseMoveEvent.hpp"
#include "events/input/MouseScrollEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"

namespace TechEngine {
    Panel::Panel(const std::string& name, SystemsRegistry& editorRegistry) : editorRegistry(editorRegistry), name(name),
                                                                             keysPressed() {
    }

    Panel::~Panel() {
    }

    void Panel::init() {
        EventDispatcher& eventDispatcher = editorRegistry.getSystem<EventDispatcher>();
        eventDispatcher.subscribe(KeyPressedEvent::eventType, [this](TechEngine::Event* event) {
            onKeyPressedEvent(((KeyPressedEvent*)event)->getKey());
        });

        eventDispatcher.subscribe(KeyReleasedEvent::eventType, [this](TechEngine::Event* event) {
            onKeyReleasedEvent(((KeyReleasedEvent*)event)->getKey());
        });

        eventDispatcher.subscribe(MouseMoveEvent::eventType, [this](TechEngine::Event* event) {
            onMouseMoveEvent(((MouseMoveEvent*)event)->getDelta());
        });

        eventDispatcher.subscribe(MouseScrollEvent::eventType, [this](TechEngine::Event* event) {
            onMouseScrollEvent(((MouseScrollEvent*)event)->getXOffset(), ((MouseScrollEvent*)event)->getYOffset());
        });
    }

    void Panel::update(ImGuiStyleVar ImGuiStyleVar_WindowPadding, const ImVec2& valImVec2, ImGuiWindowFlags flags, bool closable) {
        if (m_open) {
            if (ImGuiStyleVar_WindowPadding != -1) {
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, valImVec2);
            }
            if (closable) {
                ImGui::Begin(name.c_str(), &m_open, flags);
            } else {
                ImGui::Begin(name.c_str(), nullptr, flags);
            }
            if (ImGuiStyleVar_WindowPadding != -1) {
                ImGui::PopStyleVar();
            }
            onUpdate();
            shortcuts();
            ImGui::End();
        }
    }

    void Panel::onKeyPressedEvent(Key& key) {
        keysPressed.push_back(key);
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
        keysPressed.clear();
    }

    void Panel::processShortcuts() {
    }

    void Panel::open() {
        m_open = true;
    }

    bool Panel::isOpen() {
        return m_open;
    }
}
