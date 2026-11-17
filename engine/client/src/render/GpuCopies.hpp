#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/ResourceUploadRecord.hpp>

#include <map>
#include <span>
#include <vector>

namespace TechEngine {
    struct GpuCopy {
        StringId typeTag;
        ResourceId id;
        std::vector<ResourceSlotId> dependencies;
        bool released = false;
    };

    class GpuCopies {
    private:
        // Keyed by the whole slot: a released copy stays until shutdown, so a reused index can hold two.
        std::map<ResourceSlotId, GpuCopy> m_copies;

    public:
        GpuCopies() = default;

        GpuCopies(const GpuCopies&) = delete;

        GpuCopies& operator=(const GpuCopies&) = delete;

        GpuCopies(GpuCopies&&) = delete;

        GpuCopies& operator=(GpuCopies&&) = delete;

        // Appends each upload's slot and token to `finished` once its handler has stopped reading the payload.
        void apply(std::span<const ResourceUploadRecord> records, std::vector<FinishedUpload>& finished);

        const GpuCopy* findCopy(ResourceSlotId slot) const;

    private:
        void uploadMesh(const ResourceUploadRecord& record, GpuCopy& copy);

        void uploadTexture(const ResourceUploadRecord& record, GpuCopy& copy);

        void uploadMaterial(const ResourceUploadRecord& record, GpuCopy& copy);
    };
}
