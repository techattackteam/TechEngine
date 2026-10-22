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

    public:
        ClientApp(Role role, std::string title);

        ~ClientApp() override = default;

    protected:
        void startClient();

        void publishSnapshot(const SimulationContext& simulation) override;

        void mainThreadUpdate() override;

        void wakeMainThread() override;

        bool renderTiming(RenderTiming& out) const override;

        void shutdown() override;

        bool shouldClose() const override;
    };
}
