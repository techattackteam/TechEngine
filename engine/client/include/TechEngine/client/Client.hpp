#pragma once

#include <TechEngine/client/ClientError.hpp>
#include <TechEngine/core/TimingMetrics.hpp>

#include <functional>
#include <memory>
#include <string_view>
#include <system_error>

namespace TechEngine {
    struct RenderSnapshot;
    struct EngineContext;
    class InputBuffer;

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

        // Engine services and input must outlive the active session.
        std::error_code start(const EngineContext& engine, InputBuffer& input, int width, int height, std::string_view title, std::function<void()> onFailure = {});

        void waitEvents();

        void waitEvents(double timeoutSeconds);

        void wakeMain();

        void publish(const RenderSnapshot& snapshot) const;

        bool renderTiming(RenderTiming& out) const;

        void setTitle(std::string_view title) const;

        void setVSync(bool enabled) const;

        bool shouldClose() const;

        bool failed() const;

        void stop();
    };
}
