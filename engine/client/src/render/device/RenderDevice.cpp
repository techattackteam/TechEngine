#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/client/ClientError.hpp>

#include <render/device/RenderDevice.hpp>

#include <glad/gl.h>

#include <cstdint>
#include <string>

namespace TechEngine {
    RenderDevice::~RenderDevice() {
        TE_CHECK(getLiveObjectCount() == 0, "RenderDevice was not shutdown before destruction");
    }

    std::error_code RenderDevice::createBuffer(const BufferDescription& description, BufferHandle& out) {
        if (description.size == 0 || (description.usage == BufferUsage::Static && description.initialData.empty()) || (!description.initialData.empty() && description.initialData.size() != description.size)) {
            return ClientError::InvalidRenderDescription;
        }
        unsigned int id = 0;
        glCreateBuffers(1, &id);
        glNamedBufferStorage(id, description.size, description.initialData.data(), description.usage == BufferUsage::Dynamic ? GL_DYNAMIC_STORAGE_BIT : 0);
        out = m_buffers.create(GpuBuffer{id, description.size, description.usage});
        return {};
    }

    void RenderDevice::updateBuffer(const BufferHandle buffer, const std::size_t offset, const std::span<const std::byte> data) {
        const GpuBuffer* gpuBuffer = m_buffers.find(buffer);
        TE_CHECK(gpuBuffer != nullptr, "RenderDevice::updateBuffer called with a stale or invalid handle");
        if (gpuBuffer == nullptr) {
            return;
        }
        const bool dynamic = gpuBuffer->usage == BufferUsage::Dynamic;
        const bool inBounds = offset <= gpuBuffer->size && data.size() <= gpuBuffer->size - offset;

        TE_CHECK(dynamic, "RenderDevice::updateBuffer called on a non-dynamic buffer");
        TE_CHECK(inBounds, "RenderDevice::updateBuffer called with an out-of-bounds range");
        if (!dynamic || !inBounds) {
            return;
        }

        glNamedBufferSubData(gpuBuffer->id, offset, data.size(), data.data());
    }

    void RenderDevice::destroyBuffer(const BufferHandle buffer) {
        const GpuBuffer* gpuBuffer = m_buffers.find(buffer);
        TE_CHECK(gpuBuffer != nullptr, "RenderDevice::destroyBuffer called with a stale or invalid handle");
        if (gpuBuffer == nullptr) {
            return;
        }
        glDeleteBuffers(1, &gpuBuffer->id);
        m_buffers.destroy(buffer);
    }

    std::error_code RenderDevice::createTexture(const TextureDescription& description, TextureHandle& out) {
        if (description.width <= 0 || description.height <= 0 || description.mipLevels < 1) {
            return ClientError::InvalidRenderDescription;
        }
        unsigned int internalFormat = 0;
        switch (description.format) {
            case TextureFormat::Rgba8:
                internalFormat = GL_RGBA8;
                break;
            case TextureFormat::Rgba16F:
                internalFormat = GL_RGBA16F;
                break;
            case TextureFormat::Depth32F:
                internalFormat = GL_DEPTH_COMPONENT32F;
                break;
        }
        if (internalFormat == 0) {
            return ClientError::InvalidRenderDescription;
        }
        unsigned int id = 0;
        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        glTextureStorage2D(id, description.mipLevels, internalFormat, description.width, description.height);
        out = m_textures.create(GpuTexture{id, {description.width, description.height}, description.format});
        return {};
    }

    void RenderDevice::readTexture(const TextureHandle handle, const std::span<std::byte> out) const {
        const GpuTexture* texture = m_textures.find(handle);
        TE_CHECK(texture != nullptr, "RenderDevice::readTexture called with a stale or invalid handle");
        if (texture == nullptr) {
            return;
        }
        const std::size_t expected = static_cast<std::size_t>(texture->extent.width) * static_cast<std::size_t>(texture->extent.height) * getReadBytesPerPixel(texture->format);
        const bool sized = out.size() == expected;
        TE_CHECK(sized, "RenderDevice::readTexture needs {0} bytes for this texture, got {1}", expected, out.size());
        if (!sized) {
            return;
        }
        const unsigned int pixelFormat = texture->format == TextureFormat::Depth32F ? GL_DEPTH_COMPONENT : GL_RGBA;
        const unsigned int pixelType = texture->format == TextureFormat::Rgba8 ? GL_UNSIGNED_BYTE : GL_FLOAT;
        glGetTextureImage(texture->id, 0, pixelFormat, pixelType, static_cast<int>(out.size()), out.data());
    }

    void RenderDevice::destroyTexture(const TextureHandle handle) {
        const GpuTexture* gpuTexture = m_textures.find(handle);
        TE_CHECK(gpuTexture != nullptr, "RenderDevice::destroyTexture called with a stale or invalid handle");
        if (gpuTexture == nullptr) {
            return;
        }
        glDeleteTextures(1, &gpuTexture->id);
        m_textures.destroy(handle);
    }

    std::error_code RenderDevice::createProgram(const ProgramDescription& description, ProgramHandle& out) {
        if (description.vertexSource.empty() || description.fragmentSource.empty()) {
            return ClientError::InvalidRenderDescription;
        }
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        if (!compileShader(vertexShader, description.vertexSource, "vertex")) {
            glDeleteShader(vertexShader);
            return ClientError::ShaderCompileFailed;
        }
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        if (!compileShader(fragmentShader, description.fragmentSource, "fragment")) {
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            return ClientError::ShaderCompileFailed;
        }
        unsigned int program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        int linkStatus = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

        if (linkStatus == GL_FALSE) {
            int infoLogLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
            std::string infoLog(infoLogLength, '\0');
            glGetProgramInfoLog(program, infoLogLength, nullptr, infoLog.data());
            TE_LOGGER_ERROR("Failed to link program: {0}", infoLog);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            glDeleteProgram(program);
            return ClientError::ShaderLinkFailed;
        }

        glDetachShader(program, vertexShader);
        glDetachShader(program, fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        out = m_programs.create(GpuProgram{program});

        return {};
    }

    void RenderDevice::destroyProgram(const ProgramHandle program) {
        const GpuProgram* gpuProgram = m_programs.find(program);
        TE_CHECK(gpuProgram != nullptr, "RenderDevice::destroyProgram called with a stale or invalid handle");
        if (gpuProgram == nullptr) {
            return;
        }
        glDeleteProgram(gpuProgram->id);
        m_programs.destroy(program);
    }

    std::error_code RenderDevice::createPipeline(const PipelineDescription& description, PipelineHandle& out) {
        const GpuProgram* program = m_programs.find(description.program);
        if (program == nullptr) {
            return ClientError::InvalidRenderDescription;
        }

        std::array<std::uint32_t, MAX_VERTEX_BINDINGS> strides{};
        for (const VertexBinding& binding: description.vertexLayout.bindings) {
            if (binding.binding >= MAX_VERTEX_BINDINGS) {
                return ClientError::InvalidRenderDescription;
            }
            strides[binding.binding] = binding.stride;
        }

        for (const VertexAttribute& attribute: description.vertexLayout.attributes) {
            bool declared = false;
            for (const VertexBinding& binding: description.vertexLayout.bindings) {
                if (binding.binding == attribute.binding) {
                    declared = true;
                    break;
                }
            }
            if (!declared || attribute.binding >= MAX_VERTEX_BINDINGS) {
                return ClientError::InvalidRenderDescription;
            }
        }

        unsigned int vertexArray = 0;
        glCreateVertexArrays(1, &vertexArray);
        for (const VertexAttribute& attribute: description.vertexLayout.attributes) {
            glEnableVertexArrayAttrib(vertexArray, attribute.location);
            int componentCount = 0;
            switch (attribute.format) {
                case VertexFormat::Float2:
                    componentCount = 2;
                    break;
                case VertexFormat::Float3:
                    componentCount = 3;
                    break;
                case VertexFormat::Float4:
                    componentCount = 4;
                    break;
            }
            glVertexArrayAttribFormat(vertexArray, attribute.location, componentCount, GL_FLOAT, GL_FALSE, attribute.offset);
            glVertexArrayAttribBinding(vertexArray, attribute.location, attribute.binding);
        }

        out = m_pipelines.create(GpuPipeline{vertexArray, description.program, strides, description.topology, description.cull, description.frontFace, description.depth, description.blend});

        return {};
    }

    void RenderDevice::destroyPipeline(const PipelineHandle pipeline) {
        const GpuPipeline* gpuPipeline = m_pipelines.find(pipeline);
        TE_CHECK(gpuPipeline != nullptr, "RenderDevice::destroyPipeline called with a stale or invalid handle");
        if (gpuPipeline == nullptr) {
            return;
        }
        glDeleteVertexArrays(1, &gpuPipeline->vertexArray);
        m_pipelines.destroy(pipeline);
    }

    std::error_code RenderDevice::createRenderTarget(const RenderTargetDescription& description, RenderTargetHandle& out) {
        const GpuTexture* colorTexture = m_textures.find(description.color);
        if (colorTexture == nullptr) {
            return ClientError::InvalidRenderDescription;
        }

        const GpuTexture* depthTexture = nullptr;
        if (description.depth.valid()) {
            depthTexture = m_textures.find(description.depth);
            if (depthTexture == nullptr) {
                return ClientError::InvalidRenderDescription;
            }
        }

        if (colorTexture->format == TextureFormat::Depth32F || (depthTexture && depthTexture->format != TextureFormat::Depth32F)) {
            return ClientError::InvalidRenderDescription;
        }

        if (depthTexture && (colorTexture->extent.width != depthTexture->extent.width || colorTexture->extent.height != depthTexture->extent.height)) {
            return ClientError::InvalidRenderDescription;
        }

        unsigned int framebuffer = 0;
        glCreateFramebuffers(1, &framebuffer);
        glNamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0, colorTexture->id, 0);
        if (depthTexture != nullptr) {
            glNamedFramebufferTexture(framebuffer, GL_DEPTH_ATTACHMENT, depthTexture->id, 0);
        }
        const unsigned int status = glCheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            glDeleteFramebuffers(1, &framebuffer);
            return ClientError::IncompleteRenderTarget;
        }
        out = m_renderTargets.create(GpuRenderTarget{framebuffer, colorTexture->extent, description.color, description.depth});
        return {};
    }

    void RenderDevice::destroyRenderTarget(const RenderTargetHandle target) {
        const GpuRenderTarget* gpuRenderTarget = m_renderTargets.find(target);
        TE_CHECK(gpuRenderTarget != nullptr, "RenderDevice::destroyRenderTarget called with a stale or invalid handle");
        if (gpuRenderTarget == nullptr) {
            return;
        }
        glDeleteFramebuffers(1, &gpuRenderTarget->framebuffer);
        m_renderTargets.destroy(target);
    }

    void RenderDevice::beginRenderPass(const RenderPassDescription& description) {
        TE_CHECK(!m_inPass, "RenderDevice::beginRenderPass called while a pass is already open");
        if (m_inPass) {
            return;
        }
        const RenderExtent extent = description.extent;
        const bool positive = extent.width >= 0 && extent.height >= 0;
        TE_CHECK(positive, "RenderDevice::beginRenderPass called with a negative extent {0}x{1}", extent.width, extent.height);
        if (!positive) {
            return;
        }

        const GpuRenderTarget* renderTarget = nullptr;
        if (description.target.valid()) {
            renderTarget = m_renderTargets.find(description.target);
            TE_CHECK(renderTarget != nullptr, "RenderDevice::beginRenderPass called with a stale render target");
            if (renderTarget == nullptr) {
                return;
            }
            const bool fits = extent.width <= renderTarget->extent.width && extent.height <= renderTarget->extent.height;
            TE_CHECK(fits, "RenderDevice::beginRenderPass extent {0}x{1} exceeds its {2}x{3} target", extent.width, extent.height, renderTarget->extent.width, renderTarget->extent.height);
            if (!fits) {
                return;
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, renderTarget != nullptr ? renderTarget->framebuffer : 0);
        glViewport(0, 0, extent.width, extent.height);
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, 0, extent.width, extent.height);

        if (description.colorLoad == LoadAction::Clear) {
            glClearBufferfv(GL_COLOR, 0, description.clearColor.data());
        }
        if (description.depthLoad == LoadAction::Clear && (renderTarget == nullptr || renderTarget->depth.valid())) {
            // A depth clear obeys the depth write mask, which the last pipeline bound may have turned off.
            glDepthMask(GL_TRUE);
            glClearBufferfv(GL_DEPTH, 0, &description.clearDepth);
        }

        m_boundPipeline = {};
        m_boundIndexBuffer = {};
        m_inPass = true;
    }

    void RenderDevice::endRenderPass() {
        TE_CHECK(m_inPass, "RenderDevice::endRenderPass called while no pass is open");
        if (!m_inPass) {
            return;
        }
        glDisable(GL_SCISSOR_TEST);
        m_inPass = false;
    }

    void RenderDevice::bindPipeline(const PipelineHandle pipeline) {
        TE_CHECK(m_inPass, "RenderDevice::bindPipeline called while no pass is open");
        if (!m_inPass) {
            return;
        }
        const GpuPipeline* gpuPipeline = m_pipelines.find(pipeline);
        TE_CHECK(gpuPipeline != nullptr, "RenderDevice::bindPipeline called with a stale or invalid handle");
        if (gpuPipeline == nullptr) {
            return;
        }
        const GpuProgram* gpuProgram = m_programs.find(gpuPipeline->program);
        TE_CHECK(gpuProgram != nullptr, "RenderDevice::bindPipeline called with a pipeline whose program was destroyed");
        if (gpuProgram == nullptr) {
            return;
        }
        m_boundPipeline = pipeline;
        m_boundIndexBuffer = {};

        if (gpuPipeline->cull == CullMode::None) {
            glDisable(GL_CULL_FACE);
        } else {
            glEnable(GL_CULL_FACE);
            glCullFace(gpuPipeline->cull == CullMode::Back ? GL_BACK : GL_FRONT);
        }
        glFrontFace(gpuPipeline->frontFace == FrontFace::CounterClockwise ? GL_CCW : GL_CW);

        if (gpuPipeline->depth.test) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        glDepthMask(gpuPipeline->depth.write ? GL_TRUE : GL_FALSE);
        switch (gpuPipeline->depth.compare) {
            case CompareOp::Never:
                glDepthFunc(GL_NEVER);
                break;
            case CompareOp::Less:
                glDepthFunc(GL_LESS);
                break;
            case CompareOp::LessOrEqual:
                glDepthFunc(GL_LEQUAL);
                break;
            case CompareOp::Equal:
                glDepthFunc(GL_EQUAL);
                break;
            case CompareOp::Greater:
                glDepthFunc(GL_GREATER);
                break;
            case CompareOp::GreaterOrEqual:
                glDepthFunc(GL_GEQUAL);
                break;
            case CompareOp::Always:
                glDepthFunc(GL_ALWAYS);
                break;
        }

        if (gpuPipeline->blend == BlendMode::AlphaBlend) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glDisable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ZERO);
        }

        glUseProgram(gpuProgram->id);
        glBindVertexArray(gpuPipeline->vertexArray);
    }

    void RenderDevice::bindVertexBuffer(const std::uint32_t binding, const BufferHandle buffer, const std::size_t offset) {
        const GpuPipeline* gpuPipeline = getBoundPipeline("bindVertexBuffer");
        if (gpuPipeline == nullptr) {
            return;
        }
        const bool declared = binding < MAX_VERTEX_BINDINGS && gpuPipeline->strides[binding] > 0;
        TE_CHECK(declared, "RenderDevice::bindVertexBuffer binding {0} is not declared by the bound pipeline", binding);
        if (!declared) {
            return;
        }
        const GpuBuffer* gpuBuffer = m_buffers.find(buffer);
        TE_CHECK(gpuBuffer != nullptr, "RenderDevice::bindVertexBuffer called with a stale or invalid buffer handle");
        if (gpuBuffer == nullptr) {
            return;
        }
        const bool inBounds = offset <= gpuBuffer->size;
        TE_CHECK(inBounds, "RenderDevice::bindVertexBuffer offset {0} is past the buffer's {1} bytes", offset, gpuBuffer->size);
        if (!inBounds) {
            return;
        }
        glVertexArrayVertexBuffer(gpuPipeline->vertexArray, binding, gpuBuffer->id, static_cast<GLintptr>(offset), static_cast<GLsizei>(gpuPipeline->strides[binding]));
    }

    void RenderDevice::bindIndexBuffer(const BufferHandle buffer) {
        const GpuPipeline* gpuPipeline = getBoundPipeline("bindIndexBuffer");
        if (gpuPipeline == nullptr) {
            return;
        }
        const GpuBuffer* gpuBuffer = m_buffers.find(buffer);
        TE_CHECK(gpuBuffer != nullptr, "RenderDevice::bindIndexBuffer called with a stale or invalid buffer handle");
        if (gpuBuffer == nullptr) {
            return;
        }
        glVertexArrayElementBuffer(gpuPipeline->vertexArray, gpuBuffer->id);
        m_boundIndexBuffer = buffer;
    }

    void RenderDevice::bindStorageBuffer(const std::uint32_t binding, const BufferHandle buffer, const std::size_t offset, const std::size_t size) {
        const GpuBuffer* gpuBuffer = m_buffers.find(buffer);
        TE_CHECK(gpuBuffer != nullptr, "RenderDevice::bindStorageBuffer called with a stale or invalid buffer handle");
        if (gpuBuffer == nullptr) {
            return;
        }
        const bool inBounds = offset <= gpuBuffer->size && size <= gpuBuffer->size - offset;
        TE_CHECK(inBounds, "RenderDevice::bindStorageBuffer range {0}+{1} is past the buffer's {2} bytes", offset, size, gpuBuffer->size);
        if (!inBounds) {
            return;
        }
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding, gpuBuffer->id, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size));
    }

    void RenderDevice::drawIndexed(const std::uint32_t indexCount, const std::uint32_t firstIndex, const std::int32_t baseVertex) {
        const GpuPipeline* gpuPipeline = getBoundPipeline("drawIndexed");
        if (gpuPipeline == nullptr) {
            return;
        }
        const bool indexed = m_buffers.find(m_boundIndexBuffer) != nullptr;
        TE_CHECK(indexed, "RenderDevice::drawIndexed called with no live index buffer bound");
        if (!indexed) {
            return;
        }
        const unsigned int mode = gpuPipeline->topology == PrimitiveTopology::Lines ? GL_LINES : GL_TRIANGLES;
        const auto byteOffset = static_cast<std::uintptr_t>(firstIndex) * sizeof(std::uint32_t);
        glDrawElementsBaseVertex(mode, static_cast<int>(indexCount), GL_UNSIGNED_INT, reinterpret_cast<const void*>(byteOffset), baseVertex);
    }

    std::size_t RenderDevice::getLiveObjectCount() const {
        return m_buffers.size() + m_textures.size() + m_programs.size() + m_pipelines.size() + m_renderTargets.size();
    }

    void RenderDevice::shutdown() {
        m_buffers.destroyAll([](const GpuBuffer& buffer) {
            glDeleteBuffers(1, &buffer.id);
        });
        m_textures.destroyAll([](const GpuTexture& texture) {
            glDeleteTextures(1, &texture.id);
        });
        m_programs.destroyAll([](const GpuProgram& program) {
            glDeleteProgram(program.id);
        });
        m_pipelines.destroyAll([](const GpuPipeline& pipeline) {
            glDeleteVertexArrays(1, &pipeline.vertexArray);
        });
        m_renderTargets.destroyAll([](const GpuRenderTarget& renderTarget) {
            glDeleteFramebuffers(1, &renderTarget.framebuffer);
        });
        m_boundPipeline = {};
        m_boundIndexBuffer = {};
        m_inPass = false;
    }

    const GpuPipeline* RenderDevice::getBoundPipeline(const char* caller) const {
        TE_CHECK(m_inPass, "RenderDevice::{0} called while no pass is open", caller);
        if (!m_inPass) {
            return nullptr;
        }
        const GpuPipeline* gpuPipeline = m_pipelines.find(m_boundPipeline);
        TE_CHECK(gpuPipeline != nullptr, "RenderDevice::{0} called with no live pipeline bound", caller);
        return gpuPipeline;
    }

    bool RenderDevice::compileShader(unsigned int shader, std::string_view source, const char* stage) const {
        const char* sourcePtr = source.data();
        const int sourceLength = static_cast<int>(source.size());
        glShaderSource(shader, 1, &sourcePtr, &sourceLength);
        glCompileShader(shader);
        int compileStatus = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

        if (compileStatus == GL_FALSE) {
            int infoLogLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
            std::string infoLog(infoLogLength, '\0');
            glGetShaderInfoLog(shader, infoLogLength, nullptr, infoLog.data());
            TE_LOGGER_ERROR("Failed to compile {0} shader: {1}", stage, infoLog);
            return false;
        }
        return true;
    }
}
