#pragma once

#include <TechEngine/app/App.hpp>
#include <TechEngine/client/Client.hpp>

#include <string>

namespace TechEngine {
    class ClientApp : public App {
    private:
        Client m_client;
        std::string m_title;
        std::string m_appliedTitle;
        // Main thread only.
        bool m_pollNextUpdate = false;

    public:
        ClientApp(Role role, std::string title);

        ~ClientApp() override = default;

    protected:
        // The ImGui hook must outlive the client session.
        void startClient(ImGuiHook* imguiHook = nullptr);

        // Never throws, unlike shutdown(), so a subclass destructor can end the session before its ImGui hook dies.
        void stopClient();

        GLFWwindow* nativeWindow() const;

        void detachInput();

        // A close the render thread caused cannot be cancelled: shouldClose() stays true.
        void cancelClose();

        // Applied at once, so a caller that runs after mainThreadUpdate() is not one wake behind.
        void setTitle(std::string title);

        void publishSnapshot(const SimulationContext& simulation) override;

        void mainThreadUpdate() override;

        // Main thread only. Another thread wakes main through wakeMainThread().
        void requestMainFrame();

        void wakeMainThread() override;

        bool renderTiming(RenderTiming& out) const override;

        void shutdown() override;

        bool shouldClose() const override;

    private:
        void applyTitle();
    };
}
