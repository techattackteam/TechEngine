#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/texture/Texture.hpp>

#include <render/GpuCopies.hpp>
#include <render/device/RenderDevice.hpp>

#include <cstddef>

namespace TechEngine {
    std::error_code GpuCopies::initialize(RenderDevice& device) {
        m_device = &device;
        const std::error_code geometryError = m_geometry.initialize(device);
        if (geometryError) {
            return geometryError;
        }
        return m_materials.initialize(device);
    }

    void GpuCopies::apply(const std::span<const ResourceUploadRecord> records, std::vector<FinishedUpload>& finished) {
        for (const ResourceUploadRecord& record: records) {
            if (record.kind == ResourceUploadKind::Upload) {
                GpuCopy copy;
                copy.typeTag = record.typeTag;
                copy.id = record.id;
                copy.dependencies = record.dependencies;
                if (record.typeTag == MESH_TYPE_TAG) {
                    uploadMesh(record, copy);
                } else if (record.typeTag == TEXTURE_TYPE_TAG) {
                    uploadTexture(record, copy);
                } else if (record.typeTag == MATERIAL_TYPE_TAG) {
                    uploadMaterial(record, copy);
                } else {
                    TE_LOGGER_WARN("GpuCopies: no upload handler for {0}", record.typeTag);
                }
                m_copies.insert_or_assign(record.slot, std::move(copy));
                finished.push_back(FinishedUpload{record.slot, record.token});
            } else if (record.kind == ResourceUploadKind::Release) {
                auto it = m_copies.find(record.slot);
                if (it != m_copies.end()) {
                    it->second.released = true;
                } else {
                    TE_LOGGER_WARN("GpuCopies: release for unknown slot {0}", record.slot.index);
                }
            }
        }
    }

    const GpuCopy* GpuCopies::findCopy(const ResourceSlotId slot) const {
        const auto it = m_copies.find(slot);
        if (it == m_copies.end()) {
            return nullptr;
        }
        return &it->second;
    }

    const MaterialBuffer& GpuCopies::getMaterials() const {
        return m_materials;
    }

    void GpuCopies::shutdown() {
        for (auto& [slot, copy]: m_copies) {
            if (copy.texture.valid()) {
                m_device->destroyTexture(copy.texture);
                copy.texture = {};
            }
        }
        m_materials.shutdown();
        m_geometry.shutdown();
        m_copies.clear();
    }

    void GpuCopies::uploadMesh(const ResourceUploadRecord& record, GpuCopy& copy) {
        const Mesh& mesh = *static_cast<const Mesh*>(record.payload);
        const std::error_code error = m_geometry.place(mesh, copy.geometry);
        if (error) {
            TE_LOGGER_ERROR("GpuCopies: mesh {0:#018x} was not placed in the geometry buffers: {1}", record.id.value(), error.message());
        }
    }

    void GpuCopies::uploadTexture(const ResourceUploadRecord& record, GpuCopy& copy) {
        const Texture& texture = *static_cast<const Texture*>(record.payload);
        const std::error_code createError = m_device->createTexture(describeTexture(texture), copy.texture);
        if (createError) {
            TE_LOGGER_ERROR("GpuCopies: texture {0:#018x} was not created: {1}", record.id.value(), createError.message());
            return;
        }
        for (std::size_t level = 0; level < texture.mips.size(); level++) {
            m_device->updateTexture(copy.texture, static_cast<std::uint32_t>(level), texture.mips[level]);
        }
        m_device->makeTextureResident(copy.texture);
        refreshMaterialsNaming(record.slot, m_device->getBindlessHandle(copy.texture));
    }

    void GpuCopies::uploadMaterial(const ResourceUploadRecord& record, GpuCopy& copy) {
        const Material& material = *static_cast<const Material*>(record.payload);
        GpuMaterial gpuMaterial;
        gpuMaterial.albedo = material.baseColorFactor;
        gpuMaterial.emissiveFactor = material.emissiveFactor;
        gpuMaterial.alphaCutoff = material.alphaCutoff;
        gpuMaterial.occlusionStrength = material.occlusionStrength;
        gpuMaterial.roughnessFactor = material.roughnessFactor;
        gpuMaterial.metallicFactor = material.metallicFactor;
        gpuMaterial.normalScale = material.normalScale;
        gpuMaterial.shadingModel = static_cast<std::uint32_t>(material.shadingModel);
        resolveTextureSlots(material, record.dependencies, copy.textureSlots);
        for (std::size_t input = 0; input < MATERIAL_TEXTURE_INPUT_COUNT; input++) {
            const GpuCopy* textureCopy = findCopy(copy.textureSlots[input]);
            if (textureCopy != nullptr && textureCopy->texture.valid()) {
                gpuMaterial.setTextureHandle(input, m_device->getBindlessHandle(textureCopy->texture));
            }
        }

        const GpuCopy* previous = findCopy(record.slot);
        const bool reload = previous != nullptr && previous->materialRecord != DEFAULT_MATERIAL_RECORD;
        copy.materialRecord = reload ? previous->materialRecord : m_materials.allocate();
        const std::error_code writeError = m_materials.write(copy.materialRecord, gpuMaterial);
        if (writeError) {
            TE_LOGGER_ERROR("GpuCopies: material {0:#018x} was not written to record {1}: {2}", record.id.value(), copy.materialRecord, writeError.message());
            m_materials.free(copy.materialRecord);
            copy.materialRecord = DEFAULT_MATERIAL_RECORD;
        }
    }

    void GpuCopies::refreshMaterialsNaming(const ResourceSlotId texture, const std::uint64_t bindlessHandle) {
        // This scans every copy on each texture upload. If it shows up in a profile, keep a map from each texture slot to the material copies that name it.
        for (const auto& [slot, copy]: m_copies) {
            if (copy.typeTag != MATERIAL_TYPE_TAG || copy.materialRecord == DEFAULT_MATERIAL_RECORD) {
                continue;
            }
            GpuMaterial record = m_materials.getRecord(copy.materialRecord);
            bool namesTexture = false;
            for (std::size_t input = 0; input < MATERIAL_TEXTURE_INPUT_COUNT; input++) {
                if (copy.textureSlots[input] == texture) {
                    record.setTextureHandle(input, bindlessHandle);
                    namesTexture = true;
                }
            }
            if (namesTexture) {
                const std::error_code writeError = m_materials.write(copy.materialRecord, record);
                if (writeError) {
                    TE_LOGGER_ERROR("GpuCopies: material {0:#018x} was not updated with the handle of texture slot {1}: {2}", copy.id.value(), texture.index, writeError.message());
                }
            }
        }
    }

    void GpuCopies::resolveTextureSlots(const Material& material, const std::vector<ResourceSlotId>& dependencies, std::array<ResourceSlotId, MATERIAL_TEXTURE_INPUT_COUNT>& out) {
        out = {};
        std::size_t dependencyIndex = 0;
        const auto assignTextureSlot = [&](const ResourceId texture, const std::size_t input) {
            if (texture == ResourceId{}) {
                return;
            }
            const bool dependencyAvailable = dependencyIndex < dependencies.size();
            TE_CHECK(dependencyAvailable, "GpuCopies: material texture dependency list is incomplete");
            if (dependencyAvailable) {
                out[input] = dependencies[dependencyIndex];
                dependencyIndex++;
            }
        };

        constexpr std::size_t BASE_COLOR_INPUT = 0;
        constexpr std::size_t PACKED_ORM_INPUT = 1;
        constexpr std::size_t NORMAL_INPUT = 2;
        constexpr std::size_t EMISSIVE_INPUT = 3;

        assignTextureSlot(material.baseColorTexture.id, BASE_COLOR_INPUT);
        assignTextureSlot(material.occlusionRoughnessMetallicTexture.id, PACKED_ORM_INPUT);
        assignTextureSlot(material.normalTexture.id, NORMAL_INPUT);
        assignTextureSlot(material.emissiveTexture.id, EMISSIVE_INPUT);
    }

    TextureDescription GpuCopies::describeTexture(const Texture& texture) {
        TextureDescription description;
        description.width = static_cast<int>(texture.width);
        description.height = static_cast<int>(texture.height);
        description.mipLevels = static_cast<int>(texture.mips.size());
        switch (texture.format) {
            case TextureFormat::Bc7Srgb:
                description.format = GpuTextureFormat::Bc7Srgb;
                break;
            case TextureFormat::Bc7Unorm:
                description.format = GpuTextureFormat::Bc7Unorm;
                break;
            case TextureFormat::Bc5Unorm:
                description.format = GpuTextureFormat::Bc5Unorm;
                break;
            case TextureFormat::Rgb9E5:
                description.format = GpuTextureFormat::Rgb9E5;
                break;
        }
        description.wrapU = getGpuWrap(texture.sampler.wrapU);
        description.wrapV = getGpuWrap(texture.sampler.wrapV);
        description.filter = texture.sampler.filter == TextureFilter::Nearest ? GpuTextureFilter::Nearest : GpuTextureFilter::Linear;
        return description;
    }

    GpuTextureWrap GpuCopies::getGpuWrap(const TextureWrap wrap) {
        switch (wrap) {
            case TextureWrap::Repeat:
                return GpuTextureWrap::Repeat;
            case TextureWrap::Clamp:
                return GpuTextureWrap::Clamp;
            case TextureWrap::Mirror:
                return GpuTextureWrap::Mirror;
        }
        return GpuTextureWrap::Repeat;
    }
}
