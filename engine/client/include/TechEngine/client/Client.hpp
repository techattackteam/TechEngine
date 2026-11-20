#pragma once

#include <TechEngine/client/ClientError.hpp>
#include <TechEngine/core/TimingMetrics.hpp>

#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <system_error>
#include <vector>

struct GLFWwindow;

namespace TechEngine {
    struct FramePacket;
    struct ResourceUploadRecord;
    struct FinishedUpload;
    struct ViewRequest;
    struct EngineContext;
    class InputBuffer;
    class ImGuiHook;

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

        // Engine services, input and the ImGui hook must outlive the active session.
        std::error_code start(const EngineContext& engine, InputBuffer& input, int width, int height, std::string_view title, std::function<void()> onFailure = {}, ImGuiHook* imguiHook = nullptr);

        void pollEvents();

        void waitEvents();

        void waitEvents(double timeoutSeconds);

        void wakeMain();

        // Null only when every packet is out, which is misuse.
        std::unique_ptr<FramePacket> acquirePacket() const;

        void publish(std::unique_ptr<FramePacket> packet) const;

        // Leaves `records` empty. Each payload must stay alive until its upload comes back from takeFinishedUploads.
        void sendUploads(std::vector<ResourceUploadRecord>& records) const;

        void takeFinishedUploads(std::vector<FinishedUpload>& out) const;

        // Until the first call of a session, render draws the window through the main camera.
        void setViews(std::span<const ViewRequest> views) const;

        bool renderTiming(RenderTiming& out) const;

        void setTitle(std::string_view title) const;

        void setVSync(bool enabled) const;

        void detachInput();

        void cancelClose() const;

        bool shouldClose() const;

        bool failed() const;

        GLFWwindow* nativeWindow() const;

        void stop();
    };
}
