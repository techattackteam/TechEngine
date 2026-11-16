#include <TechEngine/base/diagnostics/Log.hpp>

#include <render/FrameRenderer.hpp>
#include <render/FrameView.hpp>
#include <render/device/RenderDevice.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <system_error>

namespace TechEngine {
    static constexpr std::array<float, 9> TRIANGLE_VERTICES{-0.5F, -0.5F, 0.0F, 0.5F, -0.5F, 0.0F, 0.0F, 0.5F, 0.0F};
    static constexpr std::array<std::uint32_t, 3> TRIANGLE_INDICES{0, 1, 2};
    static constexpr std::array<VertexAttribute, 1> TRIANGLE_ATTRIBUTES{VertexAttribute{0, 0, VertexFormat::Float3, 0}};
    static constexpr std::array<VertexBinding, 1> TRIANGLE_BINDINGS{VertexBinding{0, static_cast<std::uint32_t>(3 * sizeof(float))}};

    static constexpr const char* TRIANGLE_VERTEX_SOURCE = R"(
        #version 450 core
        layout(location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";

    static constexpr const char* TRIANGLE_FRAGMENT_SOURCE = R"(
        #version 450 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 0.5, 0.2, 1.0);
        }
    )";

    bool FrameRenderer::initialize(RenderDevice& device) {
        m_device = &device;
        const std::error_code vertexError = device.createBuffer(BufferDescription{sizeof(TRIANGLE_VERTICES), BufferUsage::Static, std::as_bytes(std::span{TRIANGLE_VERTICES})}, m_triangleVertexBuffer);
        if (vertexError) {
            return abandonInitialize("vertex buffer", vertexError);
        }
        const std::error_code indexError = device.createBuffer(BufferDescription{sizeof(TRIANGLE_INDICES), BufferUsage::Static, std::as_bytes(std::span{TRIANGLE_INDICES})}, m_triangleIndexBuffer);
        if (indexError) {
            return abandonInitialize("index buffer", indexError);
        }
        const std::error_code programError = device.createProgram(ProgramDescription{TRIANGLE_VERTEX_SOURCE, TRIANGLE_FRAGMENT_SOURCE}, m_triangleProgram);
        if (programError) {
            return abandonInitialize("program", programError);
        }
        PipelineDescription pipeline;
        pipeline.program = m_triangleProgram;
        pipeline.vertexLayout = VertexLayout{TRIANGLE_ATTRIBUTES, TRIANGLE_BINDINGS};
        const std::error_code pipelineError = device.createPipeline(pipeline, m_trianglePipeline);
        if (pipelineError) {
            return abandonInitialize("pipeline", pipelineError);
        }
        return true;
    }

    void FrameRenderer::draw(const FrameView& frame, const RenderTargetHandle target, const RenderExtent& extent) {
        if (m_device == nullptr || extent.width == 0 || extent.height == 0) {
            return;
        }

        m_device->beginRenderPass(RenderPassDescription{target, extent, LoadAction::Clear, frame.clearColor, StoreAction::Store, LoadAction::Clear, 1.0F});

        if (frame.drawTriangle) {
            m_device->bindPipeline(m_trianglePipeline);
            m_device->bindVertexBuffer(0, m_triangleVertexBuffer, 0);
            m_device->bindIndexBuffer(m_triangleIndexBuffer);
            m_device->drawIndexed(3, 0, 0);
        }
        m_device->endRenderPass();
    }

    void FrameRenderer::shutdown() {
        if (m_device == nullptr) {
            return;
        }
        if (m_trianglePipeline.valid()) {
            m_device->destroyPipeline(m_trianglePipeline);
            m_trianglePipeline = {};
        }
        if (m_triangleProgram.valid()) {
            m_device->destroyProgram(m_triangleProgram);
            m_triangleProgram = {};
        }
        if (m_triangleIndexBuffer.valid()) {
            m_device->destroyBuffer(m_triangleIndexBuffer);
            m_triangleIndexBuffer = {};
        }
        if (m_triangleVertexBuffer.valid()) {
            m_device->destroyBuffer(m_triangleVertexBuffer);
            m_triangleVertexBuffer = {};
        }
        m_device = nullptr;
    }

    bool FrameRenderer::abandonInitialize(const char* step, const std::error_code& error) {
        TE_LOGGER_ERROR("Failed to create the triangle's {0}: {1}", step, error.message());
        shutdown();
        return false;
    }
}
