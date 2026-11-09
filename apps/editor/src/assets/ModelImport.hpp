#pragma once

#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>

#include <assets/AssetIndex.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace TechEngine {
    class FileAccess;

    struct ModelImportRequest {
        std::string sourcePath;
        // The side of the assets folder the source file is in, which its outputs share.
        Side side = Side::Client;
        ImportSettings settings;
    };

    struct ModelImportResult {
        std::string folder;
        std::uint32_t nodeCount = 0;
        // The model first, then one record per written mesh, in the order assimp lists the meshes.
        std::vector<AssetRecord> records;
    };

    std::string modelImportFolder(std::string_view sourcePath);

    // Writes nothing when it fails. generateId must return a new id on every call.
    std::error_code importModel(FileAccess& files, const ModelImportRequest& request, const std::function<ResourceId()>& generateId, ModelImportResult& out);
}
