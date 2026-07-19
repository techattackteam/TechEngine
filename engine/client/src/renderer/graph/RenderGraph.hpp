#pragma once

#include <memory>
#include <vector>

#include "IRenderPass.hpp"

namespace TechEngine {
    struct FrameContext;
    class RenderResources;

    class RenderGraph {
    private:
        std::vector<std::unique_ptr<IRenderPass>> m_passes;

    public:
        template<typename PassType, typename... Args>
        PassType& addPass(Args&&... args) {
            auto pass = std::make_unique<PassType>(std::forward<Args>(args)...);
            PassType& ref = *pass;
            m_passes.emplace_back(std::move(pass));
            return ref;
        }

        void setup(RenderResources& resources);

        void execute(const FrameContext& frame, RenderResources& resources);

        void clear();
    };
}
