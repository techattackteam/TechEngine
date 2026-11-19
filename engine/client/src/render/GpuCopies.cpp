#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/texture/Texture.hpp>

#include <render/GpuCopies.hpp>

namespace TechEngine {
    std::error_code GpuCopies::initialize(RenderDevice& device) {
        return m_geometry.initialize(device);
    }

    void GpuCopies::apply(std::span<const ResourceUploadRecord> records, std::vector<FinishedUpload>& finished) {
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

    void GpuCopies::shutdown() {
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

    void GpuCopies::uploadTexture(const ResourceUploadRecord&, GpuCopy&) {
        // TODO(S11-T8): create the texture in its file's format, with its mips and sampler.
    }

    void GpuCopies::uploadMaterial(const ResourceUploadRecord&, GpuCopy&) {
        // TODO(S11-T8): write the material's record, finding its textures through the record's dependency slots.
    }
}
