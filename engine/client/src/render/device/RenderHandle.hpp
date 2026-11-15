#pragma once

#include <compare>
#include <cstdint>
#include <limits>

namespace TechEngine {
    inline constexpr std::uint32_t RENDER_NULL_INDEX = std::numeric_limits<std::uint32_t>::max();

    template<typename T>
    struct RenderHandle {
        std::uint32_t index = RENDER_NULL_INDEX;
        std::uint32_t generation = 0;

        constexpr bool valid() const {
            return index != RENDER_NULL_INDEX;
        }

        bool operator==(const RenderHandle&) const = default;
        auto operator<=>(const RenderHandle&) const = default;
    };

    struct RenderBufferTag {};

    struct RenderTextureTag {};

    struct RenderProgramTag {};

    struct RenderPipelineTag {};

    struct RenderTargetTag {};

    using BufferHandle = RenderHandle<RenderBufferTag>;
    using TextureHandle = RenderHandle<RenderTextureTag>;
    using ProgramHandle = RenderHandle<RenderProgramTag>;
    using PipelineHandle = RenderHandle<RenderPipelineTag>;
    using RenderTargetHandle = RenderHandle<RenderTargetTag>;
}
