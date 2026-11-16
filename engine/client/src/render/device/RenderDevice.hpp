#pragma once

#include <render/device/RenderHandle.hpp>
#include <render/device/RenderSlots.hpp>
#include <render/device/buffer/Buffer.hpp>
#include <render/device/pass/RenderPass.hpp>
#include <render/device/pipeline/Pipeline.hpp>
#include <render/device/program/Program.hpp>
#include <render/device/target/RenderTarget.hpp>
#include <render/device/texture/Texture.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <system_error>

namespace TechEngine {
    class RenderDevice {
    private:
        RenderSlots<BufferHandle, GpuBuffer> m_buffers;
        RenderSlots<TextureHandle, GpuTexture> m_textures;
        RenderSlots<ProgramHandle, GpuProgram> m_programs;
        RenderSlots<PipelineHandle, GpuPipeline> m_pipelines;
        RenderSlots<RenderTargetHandle, GpuRenderTarget> m_renderTargets;
        PipelineHandle m_boundPipeline;
        BufferHandle m_boundIndexBuffer;
        bool m_inPass = false;

    public:
        RenderDevice() = default;

        ~RenderDevice();

        RenderDevice(const RenderDevice&) = delete;

        RenderDevice& operator=(const RenderDevice&) = delete;

        RenderDevice(RenderDevice&&) = delete;

        RenderDevice& operator=(RenderDevice&&) = delete;

        std::error_code createBuffer(const BufferDescription& description, BufferHandle& out);

        void updateBuffer(BufferHandle buffer, std::size_t offset, std::span<const std::byte> data);

        void destroyBuffer(BufferHandle buffer);

        std::error_code createTexture(const TextureDescription& description, TextureHandle& out);

        void readTexture(TextureHandle handle, std::span<std::byte> out) const;

        void destroyTexture(TextureHandle handle);

        std::error_code createProgram(const ProgramDescription& description, ProgramHandle& out);

        void destroyProgram(ProgramHandle program);

        std::error_code createPipeline(const PipelineDescription& description, PipelineHandle& out);

        void destroyPipeline(PipelineHandle pipeline);

        std::error_code createRenderTarget(const RenderTargetDescription& description, RenderTargetHandle& out);

        void destroyRenderTarget(RenderTargetHandle target);

        void beginRenderPass(const RenderPassDescription& description);

        void endRenderPass();

        void bindPipeline(PipelineHandle pipeline);

        void bindVertexBuffer(std::uint32_t binding, BufferHandle buffer, std::size_t offset);

        void bindIndexBuffer(BufferHandle buffer);

        void bindStorageBuffer(std::uint32_t binding, BufferHandle buffer, std::size_t offset, std::size_t size);

        void drawIndexed(std::uint32_t indexCount, std::uint32_t firstIndex, std::int32_t baseVertex);

        std::size_t getLiveObjectCount() const;

        void shutdown();

    private:
        const GpuPipeline* getBoundPipeline(const char* caller) const;

        bool compileShader(unsigned int shader, std::string_view source, const char* stage) const;
    };
}
