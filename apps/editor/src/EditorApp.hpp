#pragma once

#include <TechEngine/core/FrameContext.hpp>

#include "TechEngine/app/App.hpp"

namespace TechEngine {
    class EditorApp : public App {

    public:
        EditorApp();

        ~EditorApp() override;

        static Role editorRole();

    protected:
        void init() override;

        void fixedUpdate(const FrameContext& frame) override;

        void update(const FrameContext& frame) override;

        void shutdown() override;
    };
}
