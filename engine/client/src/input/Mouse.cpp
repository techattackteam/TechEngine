#include "Mouse.hpp"

#include "systems/SystemsRegistry.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "Key.hpp"
#include "events/input/KeyHoldEvent.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "events/input/KeyReleasedEvent.hpp"
#include "events/input/MouseMoveEvent.hpp"

namespace TechEngine {
    Mouse::Mouse(SystemsRegistry& systemsRegistry): m_systemsRegistry(systemsRegistry) {
        systemsRegistry.getSystem<EventDispatcher>().subscribe<KeyPressedEvent>([this](const std::shared_ptr<Event>& event) {
            auto* keyPressedEvent = dynamic_cast<KeyPressedEvent*>(event.get());
            if (keyPressedEvent->getKey().getKeyCode() >= MOUSE_1 && keyPressedEvent->getKey().getKeyCode() <= MOUSE_8) {
                m_buttonsPressed[keyPressedEvent->getKey().getKeyCode() - MOUSE_1] = true;
            }
        });

        systemsRegistry.getSystem<EventDispatcher>().subscribe<KeyReleasedEvent>([this](const std::shared_ptr<Event>& event) {
            auto* keyReleasedEvent = dynamic_cast<KeyReleasedEvent*>(event.get());
            if (keyReleasedEvent->getKey().getKeyCode() >= MOUSE_1 && keyReleasedEvent->getKey().getKeyCode() <= MOUSE_8) {
                m_buttonsPressed[keyReleasedEvent->getKey().getKeyCode() - MOUSE_1] = false;
            }
        });
    }

    void Mouse::init() {
        m_buttonsPressed[0] = false;
        m_buttonsPressed[1] = false;
        m_buttonsPressed[2] = false;
        m_buttonsPressed[3] = false;
        m_buttonsPressed[4] = false;
        m_buttonsPressed[5] = false;
        m_buttonsPressed[6] = false;
        m_buttonsPressed[7] = false;

        m_lastPosition = glm::vec2(0, 0);
    }


    void Mouse::onUpdate() {
        if (m_buttonsPressed[0]) {
            m_systemsRegistry.getSystem<EventDispatcher>().dispatch<KeyHoldEvent>(Key(MOUSE_1));
        }
        if (m_buttonsPressed[1]) {
            m_systemsRegistry.getSystem<EventDispatcher>().dispatch<KeyHoldEvent>(Key(MOUSE_2));
        }
        if (m_buttonsPressed[2]) {
            m_systemsRegistry.getSystem<EventDispatcher>().dispatch<KeyHoldEvent>(Key(MOUSE_3));
        }
        if (m_buttonsPressed[3]) {
            m_systemsRegistry.getSystem<EventDispatcher>().dispatch<KeyHoldEvent>(Key(MOUSE_4));
        }
        if (m_buttonsPressed[4]) {
            m_systemsRegistry.getSystem<EventDispatcher>().dispatch<KeyHoldEvent>(Key(MOUSE_5));
        }
        if (m_buttonsPressed[5]) {
            m_systemsRegistry.getSystem<EventDispatcher>().dispatch<KeyHoldEvent>(Key(MOUSE_6));
        }
        if (m_buttonsPressed[6]) {
            m_systemsRegistry.getSystem<EventDispatcher>().dispatch<KeyHoldEvent>(Key(MOUSE_7));
        }
        if (m_buttonsPressed[7]) {
            m_systemsRegistry.getSystem<EventDispatcher>().dispatch<KeyHoldEvent>(Key(MOUSE_8));
        }
    }

    void Mouse::onMouseMove(double x, double y) {
        glm::vec2 fromPosition = m_lastPosition;
        glm::vec2 toPosition = glm::vec2(x, y);
        m_systemsRegistry.getSystem<EventDispatcher>().dispatch<MouseMoveEvent>(fromPosition, toPosition);
        m_lastPosition = toPosition;
    }
}
