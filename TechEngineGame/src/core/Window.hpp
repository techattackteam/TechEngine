#pragma once

#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "../renderer/Renderer.hpp"
#include "../events/window/WindowResizeEvent.hpp"

namespace TechEngine {
    class WindowSettings {
    public:
        inline static std::string title;
        inline static int width;
        inline static int height;
        inline static float aspectRatio;
    };

    class Window {
    private:
        WindowSettings settings;

        bool vSync = false;

        GLFWwindow *handler;

        Renderer renderer;

        void onWindowResizeEvent(WindowResizeEvent *event);

    public:
        Window(const std::string &title, int width, int height);

        ~Window();

        void onUpdate();

        void setVSync(bool enabled);

        bool isVSync();

        GLFWwindow *getHandler();

        static void windowKeyInput(int key, int action);

        Renderer &getRenderer();

    };
}


