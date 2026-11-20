#pragma once

#include <TechEngine/client/render/ImGuiHook.hpp>
#include <TechEngine/client/render/RenderView.hpp>

#include <render/device/RenderExtent.hpp>
#include <render/device/RenderHandle.hpp>

#include <cstdint>
#include <span>
#include <system_error>
#include <vector>

namespace TechEngine {
    class RenderDevice;

    struct ViewTarget {
        std::uint64_t token = 0;
        RenderExtent extent;
        TextureHandle color;
        TextureHandle depth;
        RenderTargetHandle target;
    };

    class ViewTargets {
    private:
        std::vector<ViewTarget> m_targets;
        RenderDevice* m_device = nullptr;

    public:
        ViewTargets() = default;

        ViewTargets(const ViewTargets&) = delete;

        ViewTargets& operator=(const ViewTargets&) = delete;

        ViewTargets(ViewTargets&&) = delete;

        ViewTargets& operator=(ViewTargets&&) = delete;

        void initialize(RenderDevice& device);

        // Returns the first creation failure; a view that failed has no target until a later update succeeds.
        std::error_code update(std::span<const ViewRequest> requests);

        const ViewTarget* findTarget(std::uint64_t token) const;

        void collectImGuiTextures(std::vector<ImGuiViewTexture>& out) const;

        void shutdown();

    private:
        std::error_code createTarget(std::uint64_t token, const RenderExtent& extent, ViewTarget& out) const;

        void destroyTarget(ViewTarget& target) const;
    };
}
