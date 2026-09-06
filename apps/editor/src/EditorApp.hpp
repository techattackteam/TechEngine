#pragma once

#include <TechEngine/client/Client.hpp>
#include <TechEngine/core/FrameContext.hpp>

#include "TechEngine/app/App.hpp"
#include <project/Project.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>

namespace TechEngine {
    class EditorApp : public App {
    private:
        std::filesystem::path m_projectRoot;
        Project m_project;
        Client m_client;
        std::chrono::steady_clock::time_point m_titleUpdated;
        std::uint64_t m_titleFrameCount = 0;
        std::uint64_t m_titleTicksCount = 0;

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
