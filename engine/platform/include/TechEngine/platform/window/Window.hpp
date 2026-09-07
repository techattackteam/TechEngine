#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <mutex>
#include <string_view>

namespace TechEngine {
    using GlProc = void (*)();
    using GlProcLoader = GlProc (*)(const char* name);

    struct FramebufferSize {
        int width = 0;
        int height = 0;
    };

    class Window {
    private:
        GLFWwindow* m_window = nullptr;
        mutable std::mutex m_framebufferMutex;
        FramebufferSize m_framebufferSize;
        std::mutex m_contextMutex;

    public:
        Window();

        ~Window();

        Window(const Window&) = delete;

        Window& operator=(const Window&) = delete;

        Window(Window&&) = delete;

        Window& operator=(Window&&) = delete;

        static bool initialize();

        static void terminate();

        bool open(int width, int height, std::string_view title);

        void pollEvents();

        void setTitle(std::string_view title);

        void close();

        bool shouldClose() const;

        FramebufferSize framebufferSize() const;

        void makeContextCurrent() const;

        void releaseContext();

        void swapBuffers();

        void setVSync(bool vsync);

        GlProcLoader processLoader() const;

    private:
        void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    };
}
