#pragma once

#include <TechEngine/app/App.hpp>
#include <TechEngine/client/Client.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Scene.hpp>

#include <cstddef>
#include <filesystem>
#include <string>
#include <system_error>

namespace TechEngine {
    class RuntimeApp : public App {
    private:
        Client m_client;
        std::string m_appliedTitle;
        std::filesystem::path m_folder;

    public:
        explicit RuntimeApp(std::filesystem::path folder);

        ~RuntimeApp() override = default;

        static Role runtimeRole();

    protected:
        void init() override;

        void mountAssets();

        std::error_code simulationStartupLoad(Scene& scene) override;

        void publishSnapshot(const SimulationContext& simulation) override;

        void mainThreadUpdate() override;

        void wakeMainThread() override;

        bool renderTiming(RenderTiming& out) const override;

        void shutdown() override;

        bool shouldClose() const override;

    private:
        void logSceneTree(const Scene& scene, Entity entity, std::size_t depth) const;
    };
}
