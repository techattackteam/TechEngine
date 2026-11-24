#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/ResourceUploadRecord.hpp>

#include <render/GeometryBuffers.hpp>
#include <render/MaterialBuffer.hpp>
#include <render/device/RenderHandle.hpp>
#include <render/device/texture/Texture.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <system_error>
#include <vector>

namespace TechEngine {
    class RenderDevice;
    struct FramePacket;
    struct Material;
    struct Mesh;
    struct Texture;
    enum class MaterialAlphaMode : std::uint8_t;
    enum class TextureWrap : std::uint8_t;

    struct GpuMeshSlot {
        // Relative to the mesh's own first index, as in the file.
        std::uint32_t firstIndex = 0;
        std::uint32_t indexCount = 0;
        ResourceSlotId defaultMaterial;

        bool operator==(const GpuMeshSlot&) const = default;
    };

    struct GpuCopy {
        StringId typeTag;
        ResourceId id;
        std::vector<ResourceSlotId> dependencies;
        GeometryRange geometry;
        std::vector<GpuMeshSlot> meshSlots;
        TextureHandle texture;
        std::uint32_t materialRecord = DEFAULT_MATERIAL_RECORD;
        MaterialAlphaMode alphaMode{};
        bool doubleSided = false;
        // In the record's input order. A null slot is an input that names no texture.
        std::array<ResourceSlotId, MATERIAL_TEXTURE_INPUT_COUNT> textureSlots{};
        bool released = false;
    };

    struct RetiredGpuCopy {
        GpuCopy copy;
        // The last frame that could have drawn it.
        std::uint64_t lastFrame = 0;
    };

    class GpuCopies {
    private:
        // Keyed by the whole slot: a released copy stays while a packet or another copy can still reach it, so a reused index can hold two.
        std::map<ResourceSlotId, GpuCopy> m_copies;
        std::vector<RetiredGpuCopy> m_retired;
        GeometryBuffers m_geometry;
        MaterialBuffer m_materials;
        RenderDevice* m_device = nullptr;

    public:
        GpuCopies() = default;

        GpuCopies(const GpuCopies&) = delete;

        GpuCopies& operator=(const GpuCopies&) = delete;

        GpuCopies(GpuCopies&&) = delete;

        GpuCopies& operator=(GpuCopies&&) = delete;

        std::error_code initialize(RenderDevice& device);

        // Appends each upload's slot and token to `finished` once its handler has stopped reading the payload.
        // `frame` is the frame about to be drawn, numbered from 1.
        void apply(std::span<const ResourceUploadRecord> records, std::uint64_t frame, std::vector<FinishedUpload>& finished);

        // After apply and prepareFrame, before the frame's draws. A null packet names nothing.
        void retireUnreachable(std::span<const FramePacket* const> packets, std::uint64_t frame);

        void dropFinished(std::uint64_t completedFrame);

        const GpuCopy* findCopy(ResourceSlotId slot) const;

        std::size_t getRetiredCount() const;

        const GeometryBuffers& getGeometry() const;

        const MaterialBuffer& getMaterials() const;

        void shutdown();

    private:
        void retire(GpuCopy copy, std::uint64_t frame);

        void dropCopy(GpuCopy& copy);

        void uploadMesh(const ResourceUploadRecord& record, GpuCopy& copy);

        void uploadTexture(const ResourceUploadRecord& record, GpuCopy& copy);

        void uploadMaterial(const ResourceUploadRecord& record, GpuCopy& copy);

        void refreshMaterialsNaming(ResourceSlotId texture, std::uint64_t bindlessHandle);

        static void resolveMeshSlots(const Mesh& mesh, const std::vector<ResourceSlotId>& dependencies, std::vector<GpuMeshSlot>& out);

        static void resolveTextureSlots(const Material& material, const std::vector<ResourceSlotId>& dependencies, std::array<ResourceSlotId, MATERIAL_TEXTURE_INPUT_COUNT>& out);

        static TextureDescription describeTexture(const Texture& texture);

        static GpuTextureWrap getGpuWrap(TextureWrap wrap);
    };
}
