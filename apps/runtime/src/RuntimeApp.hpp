#pragma once

#include <TechEngine/app/App.hpp>
#include <TechEngine/client/Client.hpp>

#include <optional>
#include <string>

namespace TechEngine {
    class RuntimeApp : public App {
    private:
        Client m_client;
        std::string m_appliedTitle;

    public:
        RuntimeApp();

        ~RuntimeApp() override = default;

        static Role runtimeRole();

    protected:
        void init() override;

        void configureSimulation() override;

        void publishSnapshot(const SimulationContext& simulation) override;

        void mainThreadUpdate() override;

        void wakeMainThread() override;

        std::optional<RenderTiming> renderTiming() const override;

        void shutdown() override;

        bool shouldClose() const override;
    };
}
