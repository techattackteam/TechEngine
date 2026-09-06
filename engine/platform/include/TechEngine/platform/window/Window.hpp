#pragma once

#include <string_view>

struct GLFWwindow;

namespace TechEngine {
    using GlProc = void (*)();
    using GlProcLoader = GlProc (*)(const char* name);

    class Window {
    private:
        GLFWwindow* m_window = nullptr;

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

        void makeContextCurrent() const;

        void releaseContext();

        void swapBuffers();

        GlProcLoader processLoader() const;
    };
}
