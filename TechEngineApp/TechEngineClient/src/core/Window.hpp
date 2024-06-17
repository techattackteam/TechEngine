#pragma once

#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Mouse.hpp"
#include "renderer/Renderer.hpp"

namespace TechEngine {
    class Window {
    private:
        Mouse mouse;
        EventDispatcher& eventDispatcher;
        std::string title;

        bool vSync = false;

        GLFWwindow* handler;

    public:
        Window(EventDispatcher& eventDispatcher, const std::string& title, uint32_t width, uint32_t height);

        ~Window();

        void init(const std::string& title, uint32_t width, uint32_t height);

        void onUpdate();

        void setVSync(bool enabled);

        bool isVSync();

        GLFWwindow* getHandler();

        void windowKeyInput(int key, int action);

        void changeTitle(const std::string& name);
    };
}
