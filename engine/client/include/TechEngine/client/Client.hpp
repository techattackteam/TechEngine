#pragma once

#include <memory>
#include <string_view>

namespace TechEngine {
    struct FrameCommand;

    // One client owns the process-wide GLFW lifetime. Call its lifecycle methods on main.
    class Client {
    private:
        struct State;
        std::unique_ptr<State> m_state;

    public:
        Client();

        ~Client();

        Client(const Client&) = delete;

        Client& operator=(const Client&) = delete;

        Client(Client&&) = delete;

        Client& operator=(Client&&) = delete;

        bool start(int width, int height, std::string_view title);

        void pollEvents();

        void publish(const FrameCommand& command);

        double renderFramesPerSecond() const;

        void setTitle(std::string_view title);

        bool shouldClose() const;

        void stop();
    };
}
