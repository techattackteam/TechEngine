#pragma once

#include <TechEngine/base/math/Math.hpp>

#include <render/DrawList.hpp>
#include <render/device/RenderExtent.hpp>
#include <render/device/RenderHandle.hpp>
#include <render/device/pipeline/Pipeline.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <system_error>

namespace TechEngine {
    struct FrameView;
    struct PacketCamera;
    class GpuCopies;
    class RenderDevice;

    struct FrameStreamBuffer {
        BufferHandle buffer;
        std::size_t capacity = 0;
    };

    class FrameRenderer {
    private:
        RenderDevice* m_device = nullptr;
        ProgramHandle m_forwardProgram;
        ProgramHandle m_cutoutProgram;
        ProgramHandle m_blendedProgram;
        std::array<PipelineHandle, DRAW_BUCKET_COUNT> m_pipelines{};
        DrawList m_drawList;
        FrameStreamBuffer m_frameUniforms;
        FrameStreamBuffer m_instanceWorlds;
        FrameStreamBuffer m_drawInstances;
        FrameStreamBuffer m_commands;

    public:
        FrameRenderer() = default;

        FrameRenderer(const FrameRenderer&) = delete;

        FrameRenderer& operator=(const FrameRenderer&) = delete;

        FrameRenderer(FrameRenderer&&) = delete;

        FrameRenderer& operator=(FrameRenderer&&) = delete;

        bool initialize(RenderDevice& device);

        // Binds the copies' geometry and material buffers as they stand now, so call it after this frame's uploads are applied.
        void draw(const FrameView& frame, const GpuCopies& copies, RenderTargetHandle target, const RenderExtent& extent);

        void shutdown();

    private:
        void collectDraws(const FrameView& frame, const GpuCopies& copies);

        std::error_code writeFrameStreams(const FrameView& frame, const RenderExtent& extent);

        std::error_code writeStream(FrameStreamBuffer& stream, std::span<const std::byte> data);

        void destroyStream(FrameStreamBuffer& stream);

        bool abandonInitialize(const char* step, const std::error_code& error);

        static PipelineDescription describePipeline(DrawBucket bucket, ProgramHandle forward, ProgramHandle cutout, ProgramHandle blended);

        static Mat4 getViewProjection(const PacketCamera& camera, const RenderExtent& extent);
    };
}
