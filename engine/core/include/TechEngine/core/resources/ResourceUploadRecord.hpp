#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>

#include <cstdint>
#include <vector>

namespace TechEngine {
    enum class ResourceUploadKind : std::uint8_t { Upload, Release };

    struct ResourceUploadRecord {
        ResourceUploadKind kind = ResourceUploadKind::Upload;
        ResourceSlotId slot;
        StringId typeTag;
        // A label for log lines only. A copy is never looked up by it.
        ResourceId id;
        // Null on a release. Valid only until this record's token is passed to finishUploads.
        const void* payload = nullptr;
        std::vector<ResourceSlotId> dependencies;
        std::uint64_t token = 0;
    };

    struct FinishedUpload {
        ResourceSlotId slot;
        std::uint64_t token = 0;

        bool operator==(const FinishedUpload&) const = default;
    };
}
