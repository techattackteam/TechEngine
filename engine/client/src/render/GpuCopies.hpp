#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/ResourceUploadRecord.hpp>

#include <render/GeometryBuffers.hpp>

#include <map>
#include <span>
#include <system_error>
#include <vector>

namespace TechEngine {
    class RenderDevice;

    struct GpuCopy {
        StringId typeTag;
        ResourceId id;
        std::vector<ResourceSlotId> dependencies;
        GeometryRange geometry;
        bool released = false;
    };

    class GpuCopies {
    private:
        // Keyed by the whole slot: a released copy stays until shutdown, so a reused index can hold two.
        std::map<ResourceSlotId, GpuCopy> m_copies;
        GeometryBuffers m_geometry;

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

        void shutdown();

    private:
        void uploadMesh(const ResourceUploadRecord& record, GpuCopy& copy);

        void uploadTexture(const ResourceUploadRecord& record, GpuCopy& copy);

        void uploadMaterial(const ResourceUploadRecord& record, GpuCopy& copy);
    };
}
