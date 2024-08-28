#include "Input.hpp"

#include "systems/SystemsRegistry.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "window/Window.hpp"

#include "events/input/MouseScrollEvent.hpp"
#include "events/input/KeyHoldEvent.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "events/input/KeyReleasedEvent.hpp"

#include <glfw/glfw3.h>

namespace TechEngine {
    Input::Input(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry), mouse(systemsRegistry) {
    }

    void Input::init() {
        GLFWwindow* handler = m_systemsRegistry.getSystem<Window>().getHandler();
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
            input->m_systemsRegistry.getSystem<EventDispatcher>().dispatch<MouseScrollEvent>((float)xoffset, (float)yoffset);
        });
        glfwSetCursorPosCallback(handler, [](GLFWwindow* handler, double xpos, double ypos) {
            Input* input = (Input*)glfwGetWindowUserPointer(handler);
            input->mouse.onMouseMove(xpos, ypos);
        });
    }

    void Input::onKeyInput(int key, int action) {
        switch (action) {
            case GLFW_PRESS: {
                m_systemsRegistry.getSystem<EventDispatcher>().dispatch<KeyPressedEvent>(Key(key));
                break;
            }
            case GLFW_RELEASE: {
                m_systemsRegistry.getSystem<EventDispatcher>().dispatch<KeyReleasedEvent>(Key(key));
                break;
            }
            case GLFW_REPEAT: {
                m_systemsRegistry.getSystem<EventDispatcher>().dispatch<KeyHoldEvent>(Key(key));
                break;
            }
            default: ;
        }
    }
}
