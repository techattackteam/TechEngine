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
#include <cstdint>
#include <map>
#include <span>
#include <system_error>
#include <vector>

namespace TechEngine {
    class RenderDevice;
    struct Material;
    struct Texture;
    enum class TextureWrap : std::uint8_t;

    struct GpuCopy {
        StringId typeTag;
        ResourceId id;
        std::vector<ResourceSlotId> dependencies;
        GeometryRange geometry;
        TextureHandle texture;
        std::uint32_t materialRecord = DEFAULT_MATERIAL_RECORD;
        // In the record's input order. A null slot is an input that names no texture.
        std::array<ResourceSlotId, MATERIAL_TEXTURE_INPUT_COUNT> textureSlots{};
        bool released = false;
    };

    class GpuCopies {
    private:
        // Keyed by the whole slot: a released copy stays until shutdown, so a reused index can hold two.
        std::map<ResourceSlotId, GpuCopy> m_copies;
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
        void apply(std::span<const ResourceUploadRecord> records, std::vector<FinishedUpload>& finished);

        const GpuCopy* findCopy(ResourceSlotId slot) const;

        const MaterialBuffer& getMaterials() const;

        void shutdown();

    private:
        void uploadMesh(const ResourceUploadRecord& record, GpuCopy& copy);

        void uploadTexture(const ResourceUploadRecord& record, GpuCopy& copy);

        void uploadMaterial(const ResourceUploadRecord& record, GpuCopy& copy);

        void refreshMaterialsNaming(ResourceSlotId texture, std::uint64_t bindlessHandle);

        static void resolveTextureSlots(const Material& material, const std::vector<ResourceSlotId>& dependencies, std::array<ResourceSlotId, MATERIAL_TEXTURE_INPUT_COUNT>& out);

        static TextureDescription describeTexture(const Texture& texture);

        static GpuTextureWrap getGpuWrap(TextureWrap wrap);
    };
}
