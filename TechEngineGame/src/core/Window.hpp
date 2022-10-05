#pragma once

#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "../events/window/WindowResizeEvent.hpp"

namespace TechEngine {
    class Window {
    private:
        std::string title;
        uint32_t width = 0;
        uint32_t height = 0;
        float aspectRatio = 0;

        bool vSync = false;

        GLFWwindow *handler;

        void onWindowResizeEvent(WindowResizeEvent *event);

    public:
        Window(const std::string &title, int width, int height);

        ~Window();

        void onUpdate();

        void setVSync(bool enabled);

        bool isVSync();

        GLFWwindow *getHandler();

        static void windowKeyInput(int key, int action);


    };
}


