#pragma once

#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "events/window/WindowResizeEvent.hpp"
#include "renderer/Renderer.hpp"

namespace TechEngine {
    class Window {
    private:
        Renderer renderer;

        std::string title;
        uint32_t width = 0;
        uint32_t height = 0;
        float aspectRatio = 0;

        bool vSync = false;

        GLFWwindow *handler;

        void onWindowResizeEvent(WindowResizeEvent *event);

    public:
        Window(const std::string &title, uint32_t width, uint32_t height);

        ~Window();

        void init(const std::string &title, uint32_t width, uint32_t height);

        void onUpdate();

        void setVSync(bool enabled);

        bool isVSync();

        GLFWwindow *getHandler();

        static void windowKeyInput(int key, int action);

        void changeTitle(const std::string &name);

        Renderer &getRenderer();
    };
}


