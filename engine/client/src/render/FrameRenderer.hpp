#pragma once

#include <render/device/RenderExtent.hpp>
#include <render/device/RenderHandle.hpp>

#include <system_error>

namespace TechEngine {
    struct RenderSnapshot;
    class RenderDevice;

    class FrameRenderer {
    private:
        RenderDevice* m_device = nullptr;
        BufferHandle m_triangleVertexBuffer;
        BufferHandle m_triangleIndexBuffer;
        ProgramHandle m_triangleProgram;
        PipelineHandle m_trianglePipeline;

    public:
        FrameRenderer() = default;

        FrameRenderer(const FrameRenderer&) = delete;

        FrameRenderer& operator=(const FrameRenderer&) = delete;

        FrameRenderer(FrameRenderer&&) = delete;

        FrameRenderer& operator=(FrameRenderer&&) = delete;

        bool initialize(RenderDevice& device);

        void draw(const RenderSnapshot& command, RenderTargetHandle target, const RenderExtent& extent);

        void shutdown();

    private:
        bool abandonInitialize(const char* step, const std::error_code& error);
    };
}
