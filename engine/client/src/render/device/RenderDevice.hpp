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

#pragma region Buffers
        std::error_code createBuffer(const BufferDescription& description, BufferHandle& out);

        void updateBuffer(BufferHandle buffer, std::size_t offset, std::span<const std::byte> data);

        void destroyBuffer(BufferHandle buffer);
#pragma endregion

#pragma region Textures
        std::error_code createTexture(const TextureDescription& description, TextureHandle& out);

        void updateTexture(TextureHandle texture, std::uint32_t level, std::span<const std::byte> data);

        void makeTextureResident(TextureHandle texture);

        // 0 until makeTextureResident.
        std::uint64_t getBindlessHandle(TextureHandle texture) const;

        void readTexture(TextureHandle handle, std::span<std::byte> out) const;

        void destroyTexture(TextureHandle handle);
#pragma endregion

#pragma region Programs
        std::error_code createProgram(const ProgramDescription& description, ProgramHandle& out);

        void destroyProgram(ProgramHandle program);
#pragma endregion

#pragma region Pipelines
        std::error_code createPipeline(const PipelineDescription& description, PipelineHandle& out);

        void destroyPipeline(PipelineHandle pipeline);
#pragma endregion

#pragma region Render targets
        std::error_code createRenderTarget(const RenderTargetDescription& description, RenderTargetHandle& out);

        void destroyRenderTarget(RenderTargetHandle target);
#pragma endregion

#pragma region Passes
        void beginRenderPass(const RenderPassDescription& description);

        void endRenderPass();
#pragma endregion

#pragma region Drawing
        void bindPipeline(PipelineHandle pipeline);

        void bindVertexBuffer(std::uint32_t binding, BufferHandle buffer, std::size_t offset);

        void bindIndexBuffer(BufferHandle buffer);

        void bindStorageBuffer(std::uint32_t binding, BufferHandle buffer, std::size_t offset, std::size_t size);

        void drawIndexed(std::uint32_t indexCount, std::uint32_t firstIndex, std::int32_t baseVertex);
#pragma endregion

#pragma region Lifetime
        std::size_t getLiveObjectCount() const;

        void shutdown();
#pragma endregion

    private:
        const GpuPipeline* getBoundPipeline(const char* caller) const;

        unsigned int getGlTextureFormat(GpuTextureFormat format) const;

        int getGlTextureWrap(GpuTextureWrap wrap) const;

        bool compileShader(unsigned int shader, std::string_view source, const char* stage) const;
    };
}
