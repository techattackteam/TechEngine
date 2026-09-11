#pragma once

#include <TechEngine/app/App.hpp>
#include <TechEngine/client/Client.hpp>

#include <project/Project.hpp>

#include <filesystem>

namespace TechEngine {
    class EditorApp : public App {
    private:
        std::filesystem::path m_projectRoot;
        Project m_project;
        Client m_client;
        std::string m_appliedTitle;

    public:
        explicit EditorApp(std::filesystem::path projectRoot);
        ~EditorApp() override = default;
        static Role editorRole();

    protected:
        void init() override;

        void publishSnapshot(const SimulationContext& simulation) override;

        void mainUpdate() override;

        void wakeMain() override;

        std::optional<RenderTiming> renderTiming() const override;

        void shutdown() override;

        bool shouldClose() const override;
    };
}
