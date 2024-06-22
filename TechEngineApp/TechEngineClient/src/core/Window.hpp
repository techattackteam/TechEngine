#pragma once

#include "Mouse.hpp"
#include "renderer/Renderer.hpp"

#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>


namespace TechEngine {
    class CLIENT_DLL Window {
    private:
        Mouse mouse;
        SystemsRegistry& systemsRegistry;
        std::string title;

        bool vSync = false;

        GLFWwindow* handler;

    public:
        Window(SystemsRegistry& systemsRegistry, const std::string& title, uint32_t width, uint32_t height);

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
