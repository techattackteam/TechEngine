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

    struct ViewDraw {
        RenderTargetHandle target;
        RenderExtent extent;
        // Null clears the target and draws nothing.
        const PacketCamera* camera = nullptr;
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
        bool m_prepared = false;

    public:
        FrameRenderer() = default;

        FrameRenderer(const FrameRenderer&) = delete;

        FrameRenderer& operator=(const FrameRenderer&) = delete;

        FrameRenderer(FrameRenderer&&) = delete;

        FrameRenderer& operator=(FrameRenderer&&) = delete;

        bool initialize(RenderDevice& device);

        // Once per frame, after this frame's uploads are applied. Every draw until the next prepare must pass the same frame and copies.
        void prepare(const FrameView& frame, const GpuCopies& copies);

        void draw(const FrameView& frame, const GpuCopies& copies, const ViewDraw& view);

        void shutdown();

    private:
        void collectDraws(const FrameView& frame, const GpuCopies& copies);

        std::error_code writeInstanceStreams(const FrameView& frame);

        std::error_code writeStream(FrameStreamBuffer& stream, std::span<const std::byte> data);

        void destroyStream(FrameStreamBuffer& stream);

        bool abandonInitialize(const char* step, const std::error_code& error);

        static PipelineDescription describePipeline(DrawBucket bucket, ProgramHandle forward, ProgramHandle cutout, ProgramHandle blended);

        static Mat4 getViewProjection(const PacketCamera& camera, const RenderExtent& extent);
    };
}
