#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/material/Material.hpp>

#include <render/FrameRenderer.hpp>
#include <render/FrameView.hpp>
#include <render/GeometryBuffers.hpp>
#include <render/GpuCopies.hpp>
#include <render/device/RenderDevice.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <system_error>

namespace TechEngine {
    struct ForwardFrameUniforms {
        Mat4 viewProjection = Mat4(1.0f);
    };

    static constexpr std::uint32_t FRAME_UNIFORM_BINDING = 0;
    static constexpr std::uint32_t MATERIAL_STORAGE_BINDING = 0;
    static constexpr std::uint32_t INSTANCE_WORLD_STORAGE_BINDING = 1;
    static constexpr std::uint32_t DRAW_INSTANCE_STORAGE_BINDING = 2;

    static constexpr std::uint32_t POSITION_BINDING = 0;
    static constexpr std::uint32_t ATTRIBUTE_BINDING = 1;

    static constexpr std::array<VertexAttribute, 4> GEOMETRY_ATTRIBUTES{
        VertexAttribute{0, POSITION_BINDING, VertexFormat::Float3, 0},
        VertexAttribute{1, ATTRIBUTE_BINDING, VertexFormat::Float3, static_cast<std::uint32_t>(offsetof(GeometryAttributes, normal))},
        VertexAttribute{2, ATTRIBUTE_BINDING, VertexFormat::Float4, static_cast<std::uint32_t>(offsetof(GeometryAttributes, tangent))},
        VertexAttribute{3, ATTRIBUTE_BINDING, VertexFormat::Float2, static_cast<std::uint32_t>(offsetof(GeometryAttributes, uv0))},
    };
    static constexpr std::array<VertexBinding, 2> GEOMETRY_BINDINGS{
        VertexBinding{POSITION_BINDING, static_cast<std::uint32_t>(sizeof(Vec3))},
        VertexBinding{ATTRIBUTE_BINDING, static_cast<std::uint32_t>(sizeof(GeometryAttributes))},
    };

    static constexpr const char* FORWARD_VERSION = "#version 460 core\n#extension GL_ARB_bindless_texture : require\n";

    static constexpr const char* CUTOUT_DEFINE = "#define TE_ALPHA_CUTOUT\n";

    static constexpr const char* BLENDED_DEFINE = "#define TE_ALPHA_BLENDED\n";

    static constexpr const char* FORWARD_VERTEX_SOURCE = R"(
        layout(location = 0) in vec3 inPosition;
        layout(location = 1) in vec3 inNormal;
        layout(location = 2) in vec4 inTangent;
        layout(location = 3) in vec2 inUv0;

        layout(std140, binding = 0) uniform FrameUniforms {
            mat4 viewProjection;
        };

        layout(std430, binding = 1) readonly buffer InstanceWorlds {
            mat4 instanceWorlds[];
        };

        layout(std430, binding = 2) readonly buffer DrawInstances {
            uvec2 drawInstances[];
        };

        out vec2 uv0;
        flat out uint materialRecord;

        void main() {
            uvec2 drawInstance = drawInstances[gl_BaseInstance + gl_InstanceID];
            uv0 = inUv0;
            materialRecord = drawInstance.y;
            gl_Position = viewProjection * instanceWorlds[drawInstance.x] * vec4(inPosition, 1.0);
        }
    )";

    static constexpr const char* FORWARD_FRAGMENT_SOURCE = R"(
        struct GpuMaterial {
            vec4 baseColorFactor;
            vec3 emissiveFactor;
            float alphaCutoff;
            float occlusionStrength;
            float roughness;
            float metallic;
            float normalScale;
            uvec2 textureHandles[4];
            uint shadingModel;
            uint padding0;
            uint padding1;
            uint padding2;
        };

        layout(std430, binding = 0) readonly buffer Materials {
            GpuMaterial materials[];
        };

        in vec2 uv0;
        flat in uint materialRecord;

        out vec4 outColor;

        vec3 linearToSrgb(vec3 linear) {
            vec3 lower = 12.92 * linear;
            vec3 upper = 1.055 * pow(max(linear, vec3(0.0)), vec3(1.0 / 2.4)) - 0.055;
            return mix(lower, upper, step(vec3(0.0031308), linear));
        }

        void main() {
            GpuMaterial material = materials[materialRecord];
            vec4 baseColor = material.baseColorFactor;
            uvec2 baseColorHandle = material.textureHandles[0];
            if (baseColorHandle != uvec2(0)) {
                sampler2D baseColorTexture = sampler2D(baseColorHandle);
                baseColor *= texture(baseColorTexture, uv0);
            }

        #ifdef TE_ALPHA_CUTOUT
            if (baseColor.a < material.alphaCutoff) {
                discard;
            }
        #endif

        #ifdef TE_ALPHA_BLENDED
            outColor = vec4(linearToSrgb(baseColor.rgb), baseColor.a);
        #else
            outColor = vec4(linearToSrgb(baseColor.rgb), 1.0);
        #endif
        }
    )";

    bool FrameRenderer::initialize(RenderDevice& device) {
        m_device = &device;
        const std::string vertexSource = std::string(FORWARD_VERSION) + FORWARD_VERTEX_SOURCE;
        const std::string fragmentSource = std::string(FORWARD_VERSION) + FORWARD_FRAGMENT_SOURCE;
        const std::string cutoutFragmentSource = std::string(FORWARD_VERSION) + CUTOUT_DEFINE + FORWARD_FRAGMENT_SOURCE;
        const std::string blendedFragmentSource = std::string(FORWARD_VERSION) + BLENDED_DEFINE + FORWARD_FRAGMENT_SOURCE;
        const std::error_code forwardError = device.createProgram(ProgramDescription{vertexSource, fragmentSource}, m_forwardProgram);
        if (forwardError) {
            return abandonInitialize("forward program", forwardError);
        }
        const std::error_code cutoutError = device.createProgram(ProgramDescription{vertexSource, cutoutFragmentSource}, m_cutoutProgram);
        if (cutoutError) {
            return abandonInitialize("cut-out program", cutoutError);
        }
        const std::error_code blendedError = device.createProgram(ProgramDescription{vertexSource, blendedFragmentSource}, m_blendedProgram);
        if (blendedError) {
            return abandonInitialize("blended program", blendedError);
        }
        for (std::size_t bucket = 0; bucket < DRAW_BUCKET_COUNT; bucket++) {
            const PipelineDescription pipeline = describePipeline(static_cast<DrawBucket>(bucket), m_forwardProgram, m_cutoutProgram, m_blendedProgram);
            const std::error_code pipelineError = device.createPipeline(pipeline, m_pipelines[bucket]);
            if (pipelineError) {
                return abandonInitialize("pipeline", pipelineError);
            }
        }
        return true;
    }

    void FrameRenderer::prepare(const FrameView& frame, const GpuCopies& copies) {
        m_prepared = false;
        if (m_device == nullptr) {
            return;
        }
        collectDraws(frame, copies);
        m_drawList.build();
        if (m_drawList.getBatches().empty()) {
            return;
        }
        const std::error_code writeError = writeInstanceStreams(frame);
        if (writeError) {
            TE_LOGGER_ERROR("The forward pass skipped a frame because a per-frame buffer was not written: {0}", writeError.message());
            return;
        }
        m_prepared = true;
    }

    void FrameRenderer::draw(const FrameView& frame, const GpuCopies& copies, const ViewDraw& view) {
        if (m_device == nullptr || view.extent.width == 0 || view.extent.height == 0) {
            return;
        }

        m_device->beginRenderPass(RenderPassDescription{view.target, view.extent, LoadAction::Clear, frame.clearColor, StoreAction::Store, LoadAction::Clear, 1.0F});

        if (view.camera != nullptr && m_prepared) {
            ForwardFrameUniforms uniforms;
            uniforms.viewProjection = getViewProjection(*view.camera, view.extent);
            const std::error_code uniformsError = writeStream(m_frameUniforms, std::as_bytes(std::span{&uniforms, 1}));
            if (uniformsError) {
                TE_LOGGER_ERROR("The forward pass skipped a view because its uniforms were not written: {0}", uniformsError.message());
                m_device->endRenderPass();
                return;
            }

            m_device->bindUniformBuffer(FRAME_UNIFORM_BINDING, m_frameUniforms.buffer, 0, sizeof(ForwardFrameUniforms));
            m_device->bindStorageBuffer(INSTANCE_WORLD_STORAGE_BINDING, m_instanceWorlds.buffer, 0, std::span{frame.instanceWorlds}.size_bytes());
            m_device->bindStorageBuffer(DRAW_INSTANCE_STORAGE_BINDING, m_drawInstances.buffer, 0, std::span{m_drawList.getInstances()}.size_bytes());
            m_device->bindStorageBuffer(MATERIAL_STORAGE_BINDING, copies.getMaterials().getBuffer(), 0, copies.getMaterials().getBufferSize());

            const GeometryBuffers& geometry = copies.getGeometry();
            for (const DrawBatch& batch: m_drawList.getBatches()) {
                m_device->bindPipeline(m_pipelines[static_cast<std::size_t>(batch.bucket)]);
                const GeometryBlock& block = geometry.getBlock(batch.block);
                m_device->bindVertexBuffer(POSITION_BINDING, block.positionsBuffer, 0);
                m_device->bindVertexBuffer(ATTRIBUTE_BINDING, block.attributesBuffer, 0);
                m_device->bindIndexBuffer(block.indexBuffer);
                m_device->drawIndexedIndirect(m_commands.buffer, batch.firstCommand * sizeof(IndexedIndirectCommand), batch.commandCount);
            }
        }
        m_device->endRenderPass();
    }

    void FrameRenderer::shutdown() {
        if (m_device == nullptr) {
            return;
        }
        destroyStream(m_commands);
        destroyStream(m_drawInstances);
        destroyStream(m_instanceWorlds);
        destroyStream(m_frameUniforms);
        for (PipelineHandle& pipeline: m_pipelines) {
            if (pipeline.valid()) {
                m_device->destroyPipeline(pipeline);
                pipeline = {};
            }
        }
        if (m_blendedProgram.valid()) {
            m_device->destroyProgram(m_blendedProgram);
            m_blendedProgram = {};
        }
        if (m_cutoutProgram.valid()) {
            m_device->destroyProgram(m_cutoutProgram);
            m_cutoutProgram = {};
        }
        if (m_forwardProgram.valid()) {
            m_device->destroyProgram(m_forwardProgram);
            m_forwardProgram = {};
        }
        m_drawList.clear();
        m_prepared = false;
        m_device = nullptr;
    }

    void FrameRenderer::collectDraws(const FrameView& frame, const GpuCopies& copies) {
        m_drawList.clear();
        if (frame.packet == nullptr) {
            return;
        }
        const FramePacket& packet = *frame.packet;
        for (std::size_t i = 0; i < packet.instances.size(); i++) {
            const PacketInstance& instance = packet.instances[i];
            const GpuCopy* mesh = copies.findCopy(instance.mesh.slot());
            if (mesh == nullptr || mesh->geometry.indexCount == 0) {
                continue;
            }
            const bool mirrored = DrawList::isMirrored(frame.instanceWorlds[i]);
            for (std::size_t s = 0; s < mesh->meshSlots.size(); s++) {
                const GpuMeshSlot& meshSlot = mesh->meshSlots[s];
                if (meshSlot.indexCount == 0) {
                    continue;
                }
                const ResourceSlotId materialSlot = s < instance.materialCount ? packet.materials[instance.firstMaterial + s].slot() : meshSlot.defaultMaterial;
                const GpuCopy* material = copies.findCopy(materialSlot);
                const bool usable = material != nullptr && material->materialRecord != DEFAULT_MATERIAL_RECORD;

                DrawItem item;
                item.block = mesh->geometry.block;
                item.firstIndex = mesh->geometry.firstIndex + meshSlot.firstIndex;
                item.indexCount = meshSlot.indexCount;
                item.baseVertex = static_cast<std::int32_t>(mesh->geometry.baseVertex);
                item.materialRecord = usable ? material->materialRecord : DEFAULT_MATERIAL_RECORD;
                item.instance = static_cast<std::uint32_t>(i);
                item.bucket = usable ? DrawList::selectBucket(material->alphaMode, material->doubleSided, mirrored) : DrawList::selectBucket(MaterialAlphaMode::Opaque, false, mirrored);
                m_drawList.add(item);
            }
        }
    }

    std::error_code FrameRenderer::writeInstanceStreams(const FrameView& frame) {
        const std::error_code worldsError = writeStream(m_instanceWorlds, std::as_bytes(std::span{frame.instanceWorlds}));
        if (worldsError) {
            return worldsError;
        }
        const std::error_code instancesError = writeStream(m_drawInstances, std::as_bytes(std::span{m_drawList.getInstances()}));
        if (instancesError) {
            return instancesError;
        }
        return writeStream(m_commands, std::as_bytes(std::span{m_drawList.getCommands()}));
    }

    std::error_code FrameRenderer::writeStream(FrameStreamBuffer& stream, const std::span<const std::byte> data) {
        if (data.size() > stream.capacity) {
            BufferDescription description;
            description.size = std::max(data.size(), stream.capacity * 2);
            description.usage = BufferUsage::Dynamic;
            BufferHandle buffer;
            const std::error_code createError = m_device->createBuffer(description, buffer);
            if (createError) {
                return createError;
            }
            // GL keeps a deleted buffer alive until the frames that bound it have finished.
            if (stream.buffer.valid()) {
                m_device->destroyBuffer(stream.buffer);
            }
            stream.buffer = buffer;
            stream.capacity = description.size;
        }
        m_device->updateBuffer(stream.buffer, 0, data);
        return {};
    }

    void FrameRenderer::destroyStream(FrameStreamBuffer& stream) {
        if (stream.buffer.valid()) {
            m_device->destroyBuffer(stream.buffer);
            stream.buffer = {};
        }
        stream.capacity = 0;
    }

    bool FrameRenderer::abandonInitialize(const char* step, const std::error_code& error) {
        TE_LOGGER_ERROR("Failed to create the forward pass's {0}: {1}", step, error.message());
        shutdown();
        return false;
    }

    PipelineDescription FrameRenderer::describePipeline(const DrawBucket bucket, const ProgramHandle forward, const ProgramHandle cutout, const ProgramHandle blended) {
        PipelineDescription description;
        description.vertexLayout = VertexLayout{GEOMETRY_ATTRIBUTES, GEOMETRY_BINDINGS};
        const bool isCutout = bucket == DrawBucket::Cutout || bucket == DrawBucket::CutoutMirrored || bucket == DrawBucket::CutoutDoubleSided;
        const bool isBlended = bucket == DrawBucket::Blended || bucket == DrawBucket::BlendedMirrored || bucket == DrawBucket::BlendedDoubleSided;
        const bool isMirrored = bucket == DrawBucket::OpaqueMirrored || bucket == DrawBucket::CutoutMirrored || bucket == DrawBucket::BlendedMirrored;
        const bool isDoubleSided = bucket == DrawBucket::OpaqueDoubleSided || bucket == DrawBucket::CutoutDoubleSided || bucket == DrawBucket::BlendedDoubleSided;
        description.program = isCutout ? cutout : (isBlended ? blended : forward);
        description.cull = isDoubleSided ? CullMode::None : CullMode::Back;
        description.frontFace = isMirrored ? FrontFace::Clockwise : FrontFace::CounterClockwise;
        description.depth = DepthState{true, !isBlended, CompareOp::Less};
        description.blend = isBlended ? BlendMode::AlphaBlend : BlendMode::Opaque;
        return description;
    }

    Mat4 FrameRenderer::getViewProjection(const PacketCamera& camera, const RenderExtent& extent) {
        const float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
        const Mat4 projection = glm::perspective(glm::radians(camera.settings.verticalFieldOfView), aspect, camera.settings.nearPlane, camera.settings.farPlane);
        return projection * glm::inverse(camera.world);
    }
}
