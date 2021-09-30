#pragma once

#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "../renderer/Renderer.hpp"

namespace Engine {
    class Window {
    private:
        std::string title;
        int width;
        int height;

        bool vSync = false;

        GLFWwindow *handler;

        Renderer renderer;

    public:
        Window(std::string title, int width, int height);

        ~Window();

        void onUpdate();

        void setVSync(bool enabled);

        bool isVSync();

        std::string &getTitle();

        GLFWwindow *getHandler();

        static void windowKeyInput(int key, int action);

        Renderer &getRenderer();
    };
}


