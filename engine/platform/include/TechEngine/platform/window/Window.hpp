#pragma once

#include <mutex>
#include <string_view>

struct GLFWwindow;

namespace TechEngine {
    class InputBuffer;
    struct InputEvent;
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
        InputBuffer* m_input = nullptr;
        double m_cursorX = 0.0;
        double m_cursorY = 0.0;
        bool m_cursorKnown = false;

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

        void waitEvents();

        void waitEvents(double timeoutSeconds);

        // Callable from any thread, but only between initialize() and terminate().
        static void postEmptyEvent();

        // The buffer must outlive this window; its callbacks publish on the event-pumping thread.
        void setInputBuffer(InputBuffer* input);

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
        void framebufferSizeCallback(int width, int height);

        void cursorPositionCallback(double x, double y);

        void publishInput(const InputEvent& event);
    };
}
