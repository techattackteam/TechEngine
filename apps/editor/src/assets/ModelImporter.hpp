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
    class MaterialImporter;
    class MeshImporter;
    struct MaterialPlan;
    struct Mesh;
    struct MeshSlot;
    struct Model;
    struct ModelNode;
    struct TransformValues;

    enum class ImportMode : std::uint8_t {
        Model,
        MergedMesh,
    };

    struct ModelImportRequest {
        std::string sourcePath;
        // The side of the assets folder the source file is in, which its outputs share.
        Side side = Side::Client;
        ImportSettings settings;
        ImportMode mode = ImportMode::Model;
    };

    struct ModelImportResult {
        std::string folder;
        // Zero for a merged mesh.
        std::uint32_t nodeCount = 0;
        // The root asset first: the model, then one record per written mesh in the order assimp lists the meshes, or the
        // merged mesh alone. Then each written texture and each material. A re-import lists only the files with a new id.
        std::vector<AssetRecord> records;
        // The files a re-import wrote again under the id they already had. Empty for an import.
        std::vector<ResourceId> rewritten;
    };

    class ModelImporter {
    private:
        struct SourceMesh;
        struct MergedPart;
        struct ExistingFolder;

        FileAccess* m_files = nullptr;
        std::function<ResourceId()> m_generateId;

    public:
        // generateId must return a new id on every call.
        ModelImporter(FileAccess& files, std::function<ResourceId()> generateId);

        static std::string importFolder(std::string_view sourcePath);

        static std::string rootAssetPath(std::string_view folder, ImportMode mode);

        // Refuses a folder that exists. Writes nothing when it fails. Of two imports of one source at once, exactly one
        // succeeds, and the other gets ImportFolderExists and leaves the winner's folder alone.
        std::error_code importFile(const ModelImportRequest& request, ModelImportResult& out) const;

        // Into the folder an earlier import wrote, with the settings of its root asset instead of the request's. An output
        // that matches an old file keeps its id and path, and a matched material is not written. Never removes a file, so
        // a failure part-way leaves the outputs written before it. Two re-imports of one source must not run at once, or
        // each gives a new output an id of its own.
        std::error_code reimportFile(const ModelImportRequest& request, ModelImportResult& out) const;

    private:
        std::error_code importInto(const ModelImportRequest& request, ExistingFolder* existing, ModelImportResult& out) const;

        std::error_code readExistingFolder(const std::string& folder, std::string_view sourcePath, ImportMode mode, ExistingFolder& out) const;

        std::uint32_t readMeshes(const aiScene& scene, std::string_view sourcePath, const std::string& folder, const MeshImporter& meshImporter, std::vector<std::string> takenFileNames, std::vector<SourceMesh>& out) const;

        std::error_code importMergedMesh(
            const aiScene& scene,
            const ModelImportRequest& request,
            const ImportSettings& settings,
            std::string_view sourcePath,
            const std::string& folder,
            const MeshImporter& meshImporter,
            const MaterialImporter& materialImporter,
            ExistingFolder* existing,
            ModelImportResult& out) const;

        void assignOutputs(const std::string& folder, const ExistingFolder* existing, MaterialPlan& plan) const;

        std::error_code writeMaterials(const MaterialImporter& materialImporter, const MaterialPlan& plan, Side side, const std::string& folder, std::vector<AssetRecord>& records) const;

        std::error_code createImportFolder(const std::string& folder) const;

        void removeUnfinishedFolder(const std::string& folder) const;

        std::error_code writeOutputs(const Model& model, const std::vector<SourceMesh>& meshes, const MaterialImporter& materialImporter, const MaterialPlan& materials, Side side, const std::string& folder, std::vector<AssetRecord>& records) const;

        std::error_code checkGltfExtensions(std::string_view sourcePath) const;

        static void keepExistingIds(ExistingFolder& existing, std::vector<SourceMesh>& meshes, MaterialPlan& plan);

        static void finishReimport(const ExistingFolder& existing, std::string_view sourcePath, std::vector<AssetRecord>& records, std::vector<ResourceId>& rewritten);

        static void buildNodes(const aiScene& scene, const std::vector<SourceMesh>& meshes, std::string_view rootName, const MeshImporter& meshImporter, std::vector<ModelNode>& out);

        static void readMergedParts(const aiScene& scene, const MeshImporter& meshImporter, std::vector<MergedPart>& out);

        static void joinParts(const aiScene& scene, const MaterialPlan& materials, std::vector<MergedPart>& parts, Mesh& out);

        static TransformValues bakeLocal(const aiNode& node, const MeshImporter& meshImporter);

        static std::string slotName(const aiScene& scene, unsigned int materialIndex);

        static std::string uniqueSlotName(const std::string& name, const std::vector<MeshSlot>& slots);

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
