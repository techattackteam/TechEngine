#pragma once

#include <TechEngine/core/TimingMetrics.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string_view>

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
        bool start(const EngineContext& engine, InputBuffer& input, int width, int height, std::string_view title, std::function<void()> onFailure = {});
        void waitEvents();

        void waitEvents(double timeoutSeconds);

        void wakeMain();

        void publish(const RenderSnapshot& snapshot) const;

        std::optional<RenderTiming> renderTiming() const;

        void setTitle(std::string_view title) const;

        void setVSync(bool enabled) const;

        bool shouldClose() const;

        bool failed() const;

        void stop();
    };
}
