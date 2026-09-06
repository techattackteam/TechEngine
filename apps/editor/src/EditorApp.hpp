#pragma once

#include <TechEngine/client/Client.hpp>
#include <TechEngine/core/FrameContext.hpp>

#include "TechEngine/app/App.hpp"
#include <project/Project.hpp>

#include <filesystem>

namespace TechEngine {
    class EditorApp : public App {
    private:
        std::filesystem::path m_projectRoot;
        Project m_project;
        Client m_client;

    public:
        explicit EditorApp(std::filesystem::path projectRoot);

        ~EditorApp() override;

        static Role editorRole();

    protected:
        void init() override;

        void fixedUpdate(const FrameContext& frame) override;

        void update(const FrameContext& frame) override;

        void shutdown() override;

        bool shouldClose() const override;
    };
}
