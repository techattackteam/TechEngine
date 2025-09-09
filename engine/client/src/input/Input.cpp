#include "Input.hpp"

#include "systems/SystemsRegistry.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "events/input/MouseScrollEvent.hpp"
#include "events/input/KeyHoldEvent.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "events/input/KeyReleasedEvent.hpp"

#include <glfw/glfw3.h>

namespace TechEngine {
    Input::Input(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry), mouse(systemsRegistry) {
    }

    void Input::init() {
        /*GLFWwindow* handler = m_systemsRegistry.getSystem<Window>().getHandler();
        glfwSetWindowUserPointer(handler, this);
        glfwSetKeyCallback(handler, [](GLFWwindow* handler, int key, int scancode, int action, int mods) {
            Input* input = (Input*)glfwGetWindowUserPointer(handler);
            input->onKeyInput(key, action);
        });
        glfwSetMouseButtonCallback(handler, [](GLFWwindow* handler, int key, int action, int mods) {
            Input* input = (Input*)glfwGetWindowUserPointer(handler);
            input->onKeyInput(key, action);
        });
        glfwSetScrollCallback(handler, [](GLFWwindow* handler, double xoffset, double yoffset) {
            Input* input = (Input*)glfwGetWindowUserPointer(handler);
            input->onMouseScroll(xoffset, yoffset);
        });
        glfwSetCursorPosCallback(handler, [](GLFWwindow* handler, double xpos, double ypos) {
            Input* input = (Input*)glfwGetWindowUserPointer(handler);
            input->onMouseMove(xpos, ypos);
        });*/
    }

    void Input::onKeyPressed(KeyCode keyCode) {
        if (keyCode < m_keyStates.size()) {
            m_keyStates[keyCode] = true;
            if (keyCode == CAPS_LOCK) {
                m_isCapsLockActive = !m_isCapsLockActive;
            }
            m_systemsRegistry.getSystem<EventDispatcher>().dispatch<KeyPressedEvent>(Key(keyCode));
        }
    }

    void Input::onKeyReleased(KeyCode keyCode) {
        if (keyCode < m_keyStates.size()) {
            m_keyStates[keyCode] = false;
            m_systemsRegistry.getSystem<EventDispatcher>().dispatch<KeyReleasedEvent>(Key(keyCode));
        }
    }

    void Input::onKeyHold(KeyCode key) {
        if (key < m_keyStates.size() && m_keyStates[key]) {
            m_systemsRegistry.getSystem<EventDispatcher>().dispatch<KeyHoldEvent>(Key(key));
        }
    }

    void Input::onMouseMove(double xpos, double ypos) {
        mouse.onMouseMove(xpos, ypos);
    }

    void Input::onMouseScroll(double xoffset, double yoffset) {
        m_systemsRegistry.getSystem<EventDispatcher>().dispatch<MouseScrollEvent>((float)xoffset, (float)yoffset);
    }

    bool Input::isKeyPressed(KeyCode key) {
        return m_keyStates[key];
    }

    bool Input::isCapsLockActive() {
        return m_isCapsLockActive;
    }

    Mouse& Input::getMouse() {
        return mouse;
    }
}
