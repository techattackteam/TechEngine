#pragma once

#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>

#include <assets/AssetIndex.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

struct aiMesh;
struct aiNode;
struct aiScene;

namespace TechEngine {
    class FileAccess;
    class MeshImporter;
    struct Model;
    struct ModelNode;
    struct TransformValues;

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

    class ModelImporter {
    private:
        struct SourceMesh;

        FileAccess* m_files = nullptr;
        std::function<ResourceId()> m_generateId;

    public:
        // generateId must return a new id on every call.
        ModelImporter(FileAccess& files, std::function<ResourceId()> generateId);

        static std::string importFolder(std::string_view sourcePath);

        // Writes nothing when it fails.
        std::error_code importFile(const ModelImportRequest& request, ModelImportResult& out) const;

    private:
        std::uint32_t readMeshes(const aiScene& scene, std::string_view sourcePath, const MeshImporter& meshImporter, std::vector<SourceMesh>& out) const;

        std::error_code writeOutputs(const Model& model, const std::vector<SourceMesh>& meshes, Side side, const std::string& folder, std::vector<AssetRecord>& records) const;

        std::error_code checkGltfExtensions(std::string_view sourcePath) const;

        static void buildNodes(const aiScene& scene, const std::vector<SourceMesh>& meshes, std::string_view rootName, const MeshImporter& meshImporter, std::vector<ModelNode>& out);

        static TransformValues bakeLocal(const aiNode& node, const MeshImporter& meshImporter);

        static std::string slotName(const aiScene& scene, const aiMesh& source);

        static char toLowerAscii(char character);

        static std::string toFileName(std::string_view name);

        static std::string uniqueFileName(std::string_view name, std::vector<std::string>& taken);

        static std::string_view fileStem(std::string_view folder);

        static bool validImportScale(float scale);

        static bool isGltfSource(std::string_view sourcePath);

        static std::uint32_t readLittleEndianU32(std::span<const std::byte> bytes, std::size_t offset);

        static bool gltfJson(std::span<const std::byte> bytes, std::string_view& out);

        static std::size_t skipJsonWhitespace(std::string_view text, std::size_t position);

        static std::size_t readJsonString(std::string_view text, std::size_t position, std::string_view& out);

        static bool arrayListsRefusedExtension(std::string_view text, std::size_t position);

        static bool listsRefusedExtension(std::string_view json);
    };
}
