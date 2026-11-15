#pragma once

#include <render/device/RenderHandle.hpp>
#include <render/device/pipeline/VertexLayout.hpp>

#include <array>
#include <cstdint>

namespace TechEngine {
    enum class PrimitiveTopology : std::uint8_t { Triangles, Lines };

    enum class CullMode : std::uint8_t { None, Back, Front };

    enum class FrontFace : std::uint8_t { CounterClockwise, Clockwise };

    enum class CompareOp : std::uint8_t { Never, Less, LessOrEqual, Equal, Greater, GreaterOrEqual, Always };

    struct DepthState {
        bool test = false;
        bool write = false;
        CompareOp compare = CompareOp::Less;
    };

    enum class BlendMode : std::uint8_t { Opaque, AlphaBlend };

    struct PipelineDescription {
        ProgramHandle program;
        VertexLayout vertexLayout;
        PrimitiveTopology topology = PrimitiveTopology::Triangles;
        CullMode cull = CullMode::None;
        FrontFace frontFace = FrontFace::CounterClockwise;
        DepthState depth;
        BlendMode blend = BlendMode::Opaque;
    };

    struct GpuPipeline {
        unsigned int vertexArray = 0;
        ProgramHandle program;
        std::array<std::uint32_t, MAX_VERTEX_BINDINGS> strides{};
        PrimitiveTopology topology = PrimitiveTopology::Triangles;
        CullMode cull = CullMode::None;
        FrontFace frontFace = FrontFace::CounterClockwise;
        DepthState depth;
        BlendMode blend = BlendMode::Opaque;
    };
}
