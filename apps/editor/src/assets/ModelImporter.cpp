#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/texture/Texture.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/platform/files/VirtualPath.hpp>

#include <assets/ImportedFile.hpp>
#include <assets/MaterialImporter.hpp>
#include <assets/MeshImporter.hpp>
#include <assets/ModelImporter.hpp>
#include <assets/ReimportMatcher.hpp>
#include <project/ProjectError.hpp>

#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    static constexpr std::string_view MESH_FOLDER = "Meshes";
    static constexpr std::string_view MATERIAL_FOLDER = "Materials";
    static constexpr std::string_view TEXTURE_FOLDER = "Textures";
    static constexpr std::string_view MATERIAL_EXTENSION = ".tematerial";
    static constexpr std::string_view TEXTURE_EXTENSION = ".tetexture";
    static constexpr std::string_view MESH_EXTENSION = ".temesh";
    static constexpr std::string_view MODEL_EXTENSION = ".temodel";
    static constexpr std::string_view UNNAMED_MESH = "Mesh";
    static constexpr std::string_view UNNAMED_NODE = "Node";
    static constexpr std::string_view UNNAMED_SLOT = "Slot ";
    static constexpr std::string_view GLTF_EXTENSION = ".gltf";
    static constexpr std::string_view GLB_EXTENSION = ".glb";
    static constexpr std::string_view GLB_MAGIC = "glTF";
    // A .glb's 12-byte header is followed by the JSON chunk's u32 length and u32 type, then the JSON itself.
    static constexpr std::size_t GLB_JSON_OFFSET = 20;
    static constexpr std::uint32_t GLB_JSON_CHUNK_TYPE = 0x4E4F534A;
    static constexpr std::string_view EXTENSIONS_USED_KEY = "\"extensionsUsed\"";
    static constexpr std::array<std::string_view, 3> REFUSED_GLTF_EXTENSIONS{"KHR_mesh_quantization", "EXT_meshopt_compression", "KHR_meshopt_compression"};
    // aiProcess_GenNormals makes flat normals only for a mesh that has none; a file's own normals are kept.
    static constexpr unsigned int IMPORT_STEPS = aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices;
    static constexpr unsigned int MERGED_IMPORT_STEPS = aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices;
    static constexpr float MIN_ROTATION_LENGTH_SQUARED = 1.0e-12f;
    static const Vec3 MERGED_MISSING_NORMAL{0.0f, 1.0f, 0.0f};
    static const Vec4 MERGED_MISSING_TANGENT{1.0f, 0.0f, 0.0f, 1.0f};
    static const Vec2 MERGED_MISSING_UV0{0.0f, 0.0f};

    struct ModelImporter::SourceMesh {
        // A null id marks a source mesh that is not written, so no node refers to it.
        Mesh mesh;
        std::string name;
        std::string fileName;
        std::string path;
        unsigned int materialIndex = 0;
    };

    struct ModelImporter::MergedPart {
        Mesh mesh;
        unsigned int materialIndex = 0;
    };

    struct ModelImporter::ExistingFolder {
        ImportSettings settings;
        // Null when the folder holds no root asset of the request's mode.
        ResourceId rootId;
        // Empty when the folder holds no root asset of the other mode.
        std::string otherRootPath;
        // Every imported file under the folder but the two root assets.
        std::vector<ImportedFile> files;
        std::vector<bool> matched;
        std::vector<ResourceId> keptIds;
        // The lowercase stems of the files in Meshes, Materials and Textures, which a new output must not take.
        std::vector<std::string> takenMeshNames;
        std::vector<std::string> takenMaterialNames;
        std::vector<std::string> takenTextureNames;
    };

    struct PendingNode {
        const aiNode* node = nullptr;
        std::uint32_t parent = MODEL_NO_PARENT;
    };

    struct PendingMergedNode {
        const aiNode* node = nullptr;
        Mat4 parentWorld = Mat4(1.0f);
    };

    // assimp's matrices are row-major and glm's are column-major.
    static Mat4 toMat4(const aiMatrix4x4& matrix) {
        return Mat4(matrix.a1, matrix.b1, matrix.c1, matrix.d1, matrix.a2, matrix.b2, matrix.c2, matrix.d2, matrix.a3, matrix.b3, matrix.c3, matrix.d3, matrix.a4, matrix.b4, matrix.c4, matrix.d4);
    }

    ModelImporter::ModelImporter(FileAccess& files, std::function<ResourceId()> generateId) : m_files(&files), m_generateId(std::move(generateId)) {
    }

    std::string ModelImporter::importFolder(const std::string_view sourcePath) {
        const std::size_t slash = sourcePath.find_last_of('/');
        const std::size_t dot = sourcePath.find_last_of('.');
        const bool hasExtension = dot != std::string_view::npos && (slash == std::string_view::npos || dot > slash + 1);
        if (!hasExtension) {
            return std::string(sourcePath);
        }
        return std::string(sourcePath.substr(0, dot));
    }

    std::string ModelImporter::rootAssetPath(const std::string_view folder, const ImportMode mode) {
        const std::string_view extension = mode == ImportMode::MergedMesh ? MESH_EXTENSION : MODEL_EXTENSION;
        return std::string(folder) + "/" + std::string(fileStem(folder)) + std::string(extension);
    }

    std::error_code ModelImporter::importFile(const ModelImportRequest& request, ModelImportResult& out) const {
        return importInto(request, nullptr, out);
    }

    std::error_code ModelImporter::reimportFile(const ModelImportRequest& request, ModelImportResult& out) const {
        ExistingFolder existing;
        return importInto(request, &existing, out);
    }

    std::error_code ModelImporter::importInto(const ModelImportRequest& request, ExistingFolder* existing, ModelImportResult& out) const {
        TE_PROFILER_FUNCTION();
        if (existing == nullptr && !validImportScale(request.settings.scale)) {
            return ProjectError::InvalidImportScale;
        }

        VirtualPathParts parts;
        const std::error_code splitError = splitVirtualPath(request.sourcePath, parts);
        if (splitError) {
            return splitError;
        }

        std::filesystem::path physicalPath;
        const std::error_code resolveError = m_files->resolve(request.sourcePath, physicalPath);
        if (resolveError) {
            return resolveError;
        }

        const std::string folder = importFolder(request.sourcePath);
        FileStatus folderStatus;
        const std::error_code folderError = m_files->status(folder, folderStatus);
        if (existing == nullptr) {
            if (!folderError) {
                return ProjectError::ImportFolderExists;
            }
            if (folderError != FileError::NotFound) {
                return folderError;
            }
        } else {
            if (folderError == FileError::NotFound || (!folderError && !folderStatus.isDirectory)) {
                return ProjectError::ReimportTargetMissing;
            }
            if (folderError) {
                return folderError;
            }
            const std::error_code existingError = readExistingFolder(folder, request.mode, *existing);
            if (existingError) {
                return existingError;
            }
            if (!validImportScale(existing->settings.scale)) {
                return ProjectError::InvalidImportScale;
            }
        }
        const ImportSettings settings = existing == nullptr ? request.settings : existing->settings;

        // assimp reads a quantized or meshopt-compressed glTF without an error, as garbage positions.
        const bool gltfSource = isGltfSource(request.sourcePath);
        if (gltfSource) {
            const std::error_code extensionError = checkGltfExtensions(request.sourcePath);
            if (extensionError) {
                return extensionError;
            }
        }

        Assimp::Importer importer;
        // Without this, every FBX node arrives as a chain of helper nodes, one per pivot.
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
        const std::u8string utf8Path = physicalPath.u8string();
        const std::string assimpPath(utf8Path.begin(), utf8Path.end());
        const unsigned int steps = request.mode == ImportMode::MergedMesh ? MERGED_IMPORT_STEPS : IMPORT_STEPS;
        const aiScene* scene = importer.ReadFile(assimpPath, steps);
        if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || scene->mRootNode == nullptr) {
            TE_LOGGER_WARN("Import: assimp did not read {0}: {1}", request.sourcePath, importer.GetErrorString());
            return ProjectError::SourceUnreadable;
        }

        const MeshImporter meshImporter(settings, gltfSource);
        const MaterialImporter materialImporter(*m_files, *scene, request.sourcePath, parts.relative, gltfSource);
        if (request.mode == ImportMode::MergedMesh) {
            return importMergedMesh(*scene, request, settings, parts.relative, folder, meshImporter, materialImporter, existing, out);
        }

        std::vector<SourceMesh> meshes;
        const std::uint32_t meshCount = readMeshes(*scene, parts.relative, folder, meshImporter, existing == nullptr ? std::vector<std::string>{} : existing->takenMeshNames, meshes);
        if (meshCount == 0) {
            return ProjectError::SourceHasNoMeshes;
        }

        std::vector<bool> usedMaterials(scene->mNumMaterials, false);
        for (const SourceMesh& mesh: meshes) {
            if (mesh.mesh.id.valid() && mesh.materialIndex < usedMaterials.size()) {
                usedMaterials[mesh.materialIndex] = true;
            }
        }
        MaterialPlan materials;
        materialImporter.plan(usedMaterials, materials);
        assignOutputs(folder, existing, materials);
        if (existing != nullptr) {
            keepExistingIds(*existing, meshes, materials);
        }
        for (SourceMesh& mesh: meshes) {
            if (mesh.mesh.id.valid()) {
                mesh.mesh.slots[0].defaultMaterial = materials.materialId(mesh.materialIndex);
            }
        }

        Model model;
        model.id = existing != nullptr && existing->rootId.valid() ? existing->rootId : m_generateId();
        model.importBlock.sourcePath = std::string(parts.relative);
        model.importBlock.settings = settings;
        buildNodes(*scene, meshes, fileStem(folder), meshImporter, model.nodes);

        if (existing == nullptr) {
            const std::error_code createError = createImportFolder(folder);
            if (createError) {
                return createError;
            }
        }

        std::vector<AssetRecord> records;
        records.reserve(static_cast<std::size_t>(meshCount) + 1);
        const std::error_code writeError = writeOutputs(model, meshes, materialImporter, materials, request.side, folder, records);
        if (writeError) {
            if (existing == nullptr) {
                removeUnfinishedFolder(folder);
            }
            return writeError;
        }

        out.folder = folder;
        out.nodeCount = static_cast<std::uint32_t>(model.nodes.size());
        out.rewritten.clear();
        if (existing != nullptr) {
            finishReimport(*existing, request.sourcePath, records, out.rewritten);
        }
        out.records = std::move(records);
        return {};
    }

    std::error_code ModelImporter::readExistingFolder(const std::string& folder, const ImportMode mode, ExistingFolder& out) const {
        std::vector<ImportedFile> files;
        const std::error_code listError = listImportedFiles(*m_files, folder, files);
        if (listError) {
            return listError;
        }

        const std::string rootPath = rootAssetPath(folder, mode);
        const std::string otherRootPath = rootAssetPath(folder, mode == ImportMode::Model ? ImportMode::MergedMesh : ImportMode::Model);
        const std::string meshFolder = folder + "/" + std::string(MESH_FOLDER);
        const std::string materialFolder = folder + "/" + std::string(MATERIAL_FOLDER);
        const std::string textureFolder = folder + "/" + std::string(TEXTURE_FOLDER);
        bool hasRoot = false;
        ImportSettings otherRootSettings;
        for (ImportedFile& file: files) {
            if (file.path == rootPath) {
                hasRoot = true;
                out.rootId = file.id;
                out.settings = file.importBlock.settings;
                continue;
            }
            if (file.path == otherRootPath) {
                out.otherRootPath = file.path;
                otherRootSettings = file.importBlock.settings;
                continue;
            }

            const std::size_t slash = file.path.find_last_of('/');
            const std::string_view parent = std::string_view(file.path).substr(0, slash);
            std::string stem = file.path.substr(slash + 1);
            stem = stem.substr(0, stem.find_last_of('.'));
            std::ranges::transform(stem, stem.begin(), toLowerAscii);
            if (parent == meshFolder) {
                out.takenMeshNames.push_back(std::move(stem));
            } else if (parent == materialFolder) {
                out.takenMaterialNames.push_back(std::move(stem));
            } else if (parent == textureFolder) {
                out.takenTextureNames.push_back(std::move(stem));
            }
            out.files.push_back(std::move(file));
        }

        if (!hasRoot && out.otherRootPath.empty()) {
            return ProjectError::ReimportRootMissing;
        }
        if (!hasRoot) {
            out.settings = otherRootSettings;
        }
        out.matched.assign(out.files.size(), false);
        return {};
    }

    // Every output is matched in one call, so each old file is kept at most once and only by an output of its kind.
    void ModelImporter::keepExistingIds(ExistingFolder& existing, std::vector<SourceMesh>& meshes, MaterialPlan& plan) {
        if (existing.rootId.valid()) {
            existing.keptIds.push_back(existing.rootId);
        }

        std::vector<ImportKey> oldKeys;
        oldKeys.reserve(existing.files.size());
        for (const ImportedFile& file: existing.files) {
            oldKeys.push_back(ImportKey{file.typeTag, file.importBlock.sourceIndex, file.importBlock.sourceName});
        }
        std::vector<ImportKey> wanted;
        for (const SourceMesh& mesh: meshes) {
            if (mesh.mesh.id.valid()) {
                wanted.push_back(ImportKey{MESH_TYPE_TAG, mesh.mesh.importBlock.sourceIndex, mesh.mesh.importBlock.sourceName});
            }
        }
        for (const TextureOutput& texture: plan.textures) {
            wanted.push_back(ImportKey{TEXTURE_TYPE_TAG, texture.texture.importBlock.sourceIndex, texture.texture.importBlock.sourceName});
        }
        for (const MaterialOutput& material: plan.materials) {
            wanted.push_back(ImportKey{MATERIAL_TYPE_TAG, material.material.importBlock.sourceIndex, material.material.importBlock.sourceName});
        }

        std::vector<std::uint32_t> matches;
        ReimportMatcher::match(oldKeys, wanted, matches);
        std::size_t next = 0;
        const auto keep = [&existing, &matches, &next](ResourceId& id, std::string& path) {
            const std::uint32_t match = matches[next];
            next++;
            if (match == REIMPORT_NO_MATCH) {
                return false;
            }
            const ImportedFile& file = existing.files[match];
            existing.matched[match] = true;
            existing.keptIds.push_back(file.id);
            id = file.id;
            path = file.path;
            return true;
        };
        for (SourceMesh& mesh: meshes) {
            if (mesh.mesh.id.valid()) {
                keep(mesh.mesh.id, mesh.path);
            }
        }
        for (TextureOutput& texture: plan.textures) {
            keep(texture.texture.id, texture.path);
        }
        for (MaterialOutput& material: plan.materials) {
            if (keep(material.material.id, material.path)) {
                material.writeFile = false;
            }
        }
    }

    // A record whose id was kept is already in the index, so it moves from records to rewritten.
    void ModelImporter::finishReimport(const ExistingFolder& existing, const std::string_view sourcePath, std::vector<AssetRecord>& records, std::vector<ResourceId>& rewritten) {
        const auto kept = [&existing](const AssetRecord& record) {
            return std::ranges::find(existing.keptIds, record.id) != existing.keptIds.end();
        };
        for (const AssetRecord& record: records) {
            if (kept(record)) {
                rewritten.push_back(record.id);
            }
        }
        std::erase_if(records, kept);

        for (std::size_t i = 0; i < existing.files.size(); i++) {
            if (!existing.matched[i]) {
                TE_LOGGER_WARN("Re-import: {0} matches no output of {1}, so it is left as it was", existing.files[i].path, sourcePath);
            }
        }
        if (!existing.otherRootPath.empty()) {
            TE_LOGGER_WARN("Re-import: {0} is the root asset of the other import mode, so it is left as it was", existing.otherRootPath);
        }
    }

    std::error_code ModelImporter::importMergedMesh(
        const aiScene& scene,
        const ModelImportRequest& request,
        const ImportSettings& settings,
        const std::string_view sourcePath,
        const std::string& folder,
        const MeshImporter& meshImporter,
        const MaterialImporter& materialImporter,
        ExistingFolder* existing,
        ModelImportResult& out) const {
        std::vector<MergedPart> parts;
        readMergedParts(scene, meshImporter, parts);
        if (parts.empty()) {
            return ProjectError::SourceHasNoMeshes;
        }

        std::vector<bool> usedMaterials(scene.mNumMaterials, false);
        for (const MergedPart& part: parts) {
            if (part.materialIndex < usedMaterials.size()) {
                usedMaterials[part.materialIndex] = true;
            }
        }
        MaterialPlan materials;
        materialImporter.plan(usedMaterials, materials);

        Mesh mesh;
        mesh.id = existing != nullptr && existing->rootId.valid() ? existing->rootId : m_generateId();
        assignOutputs(folder, existing, materials);
        if (existing != nullptr) {
            std::vector<SourceMesh> noMeshes;
            keepExistingIds(*existing, noMeshes, materials);
        }
        joinParts(scene, materials, parts, mesh);
        parts.clear();
        mesh.importBlock.sourcePath = std::string(sourcePath);
        mesh.importBlock.settings = settings;

        if (existing == nullptr) {
            const std::error_code createError = createImportFolder(folder);
            if (createError) {
                return createError;
            }
        }

        std::vector<AssetRecord> records;
        const std::error_code materialError = writeMaterials(materialImporter, materials, request.side, folder, records);
        if (materialError) {
            if (existing == nullptr) {
                removeUnfinishedFolder(folder);
            }
            return materialError;
        }

        // The mesh is written last, so a folder that holds the root asset holds every material it names.
        std::vector<std::byte> bytes;
        mesh.write(bytes);
        std::string path = rootAssetPath(folder, ImportMode::MergedMesh);
        const std::error_code writeError = m_files->write(path, bytes);
        if (writeError) {
            if (existing == nullptr) {
                removeUnfinishedFolder(folder);
            }
            return writeError;
        }
        records.insert(records.begin(), AssetRecord{mesh.id, MESH_TYPE_TAG, request.side, std::move(path)});

        out.folder = folder;
        out.nodeCount = 0;
        out.rewritten.clear();
        if (existing != nullptr) {
            finishReimport(*existing, request.sourcePath, records, out.rewritten);
        }
        out.records = std::move(records);
        return {};
    }

    void ModelImporter::assignOutputs(const std::string& folder, const ExistingFolder* existing, MaterialPlan& plan) const {
        std::vector<std::string> takenTextureNames = existing == nullptr ? std::vector<std::string>{} : existing->takenTextureNames;
        for (TextureOutput& texture: plan.textures) {
            texture.path = folder + "/" + std::string(TEXTURE_FOLDER) + "/" + uniqueFileName(texture.name, takenTextureNames) + std::string(TEXTURE_EXTENSION);
            texture.texture.id = m_generateId();
        }
        std::vector<std::string> takenMaterialNames = existing == nullptr ? std::vector<std::string>{} : existing->takenMaterialNames;
        for (MaterialOutput& material: plan.materials) {
            material.path = folder + "/" + std::string(MATERIAL_FOLDER) + "/" + uniqueFileName(material.name, takenMaterialNames) + std::string(MATERIAL_EXTENSION);
            material.material.id = m_generateId();
        }
    }

    // A Textures folder this call made, and whose every image failed to load, is removed again, so the folder holds only
    // what was written. A re-import finds the folders already there.
    std::error_code ModelImporter::writeMaterials(const MaterialImporter& materialImporter, const MaterialPlan& plan, const Side side, const std::string& folder, std::vector<AssetRecord>& records) const {
        const std::string textureFolder = folder + "/" + std::string(TEXTURE_FOLDER);
        bool createdTextureFolder = false;
        if (!plan.textures.empty()) {
            const std::error_code textureFolderError = m_files->createDirectory(textureFolder);
            if (textureFolderError && textureFolderError != FileError::AlreadyExists) {
                return textureFolderError;
            }
            createdTextureFolder = !textureFolderError;
        }
        if (!plan.materials.empty()) {
            const std::error_code materialFolderError = m_files->createDirectory(folder + "/" + std::string(MATERIAL_FOLDER));
            if (materialFolderError && materialFolderError != FileError::AlreadyExists) {
                return materialFolderError;
            }
        }

        const std::size_t firstRecord = records.size();
        const std::error_code writeError = materialImporter.write(plan, side, records);
        if (writeError) {
            return writeError;
        }
        const bool wroteTexture = std::any_of(records.begin() + static_cast<std::ptrdiff_t>(firstRecord), records.end(), [](const AssetRecord& record) {
            return record.typeTag == TEXTURE_TYPE_TAG;
        });
        if (createdTextureFolder && !wroteTexture) {
            const std::error_code removeError = m_files->remove(textureFolder, false);
            if (removeError) {
                TE_LOGGER_WARN("Import: failed to remove the empty {0}: {1}", textureFolder, removeError.message());
            }
        }
        return {};
    }

    std::error_code ModelImporter::createImportFolder(const std::string& folder) const {
        const std::error_code createError = m_files->createDirectory(folder);
        if (createError == FileError::AlreadyExists) {
            return ProjectError::ImportFolderExists;
        }
        return createError;
    }

    // A folder left behind would refuse every later import of this source.
    void ModelImporter::removeUnfinishedFolder(const std::string& folder) const {
        const std::error_code removeError = m_files->remove(folder, true);
        if (removeError) {
            TE_LOGGER_WARN("Import: failed to remove the unfinished {0}: {1}", folder, removeError.message());
        }
    }

    // takenFileNames holds the lowercase names no mesh file may take.
    std::uint32_t ModelImporter::readMeshes(const aiScene& scene, const std::string_view sourcePath, const std::string& folder, const MeshImporter& meshImporter, std::vector<std::string> takenFileNames, std::vector<SourceMesh>& out) const {
        std::uint32_t written = 0;
        out.resize(scene.mNumMeshes);
        for (unsigned int i = 0; i < scene.mNumMeshes; i++) {
            const aiMesh& source = *scene.mMeshes[i];
            SourceMesh& mesh = out[i];
            if (!meshImporter.read(source, mesh.mesh)) {
                TE_LOGGER_WARN("Import: skipped mesh {0} '{1}', because it is not made of triangles", i, source.mName.C_Str());
                continue;
            }
            mesh.mesh.id = m_generateId();
            mesh.mesh.importBlock.sourcePath = std::string(sourcePath);
            mesh.mesh.importBlock.sourceIndex = i;
            mesh.mesh.importBlock.sourceName = source.mName.C_Str();
            mesh.mesh.slots = {MeshSlot{slotName(scene, source.mMaterialIndex), 0, static_cast<std::uint32_t>(mesh.mesh.indices.size()), ResourceId{}}};
            mesh.materialIndex = source.mMaterialIndex;
            mesh.name = source.mName.C_Str();
            mesh.fileName = uniqueFileName(mesh.name, takenFileNames);
            mesh.path = folder + "/" + std::string(MESH_FOLDER) + "/" + mesh.fileName + std::string(MESH_EXTENSION);
            if (mesh.name.empty()) {
                mesh.name = mesh.fileName;
            }
            written++;
        }
        return written;
    }

    std::error_code ModelImporter::writeOutputs(const Model& model, const std::vector<SourceMesh>& meshes, const MaterialImporter& materialImporter, const MaterialPlan& materials, const Side side, const std::string& folder, std::vector<AssetRecord>& records) const {
        std::vector<AssetRecord> materialRecords;
        const std::error_code materialError = writeMaterials(materialImporter, materials, side, folder, materialRecords);
        if (materialError) {
            return materialError;
        }

        const std::string meshFolder = folder + "/" + std::string(MESH_FOLDER);
        const std::error_code meshFolderError = m_files->createDirectory(meshFolder);
        if (meshFolderError && meshFolderError != FileError::AlreadyExists) {
            return meshFolderError;
        }

        std::vector<std::byte> bytes;
        for (const SourceMesh& mesh: meshes) {
            if (!mesh.mesh.id.valid()) {
                continue;
            }
            bytes.clear();
            mesh.mesh.write(bytes);
            const std::error_code meshError = m_files->write(mesh.path, bytes);
            if (meshError) {
                return meshError;
            }
            records.push_back(AssetRecord{mesh.mesh.id, MESH_TYPE_TAG, side, mesh.path});
        }

        // The model is written last, so a folder that holds a model holds every mesh the model names.
        bytes.clear();
        model.write(bytes);
        std::string modelPath = rootAssetPath(folder, ImportMode::Model);
        const std::error_code modelError = m_files->write(modelPath, bytes);
        if (modelError) {
            return modelError;
        }
        records.insert(records.begin(), AssetRecord{model.id, MODEL_TYPE_TAG, side, std::move(modelPath)});
        records.insert(records.end(), std::make_move_iterator(materialRecords.begin()), std::make_move_iterator(materialRecords.end()));
        return {};
    }

    // A file whose JSON cannot be found is left for assimp to refuse.
    std::error_code ModelImporter::checkGltfExtensions(const std::string_view sourcePath) const {
        std::vector<std::byte> bytes;
        const std::error_code readError = m_files->read(sourcePath, bytes);
        if (readError) {
            return readError;
        }

        std::string_view json;
        if (!gltfJson(bytes, json)) {
            return {};
        }
        if (listsRefusedExtension(json)) {
            return ProjectError::SourceCompressed;
        }
        return {};
    }

    void ModelImporter::buildNodes(const aiScene& scene, const std::vector<SourceMesh>& meshes, const std::string_view rootName, const MeshImporter& meshImporter, std::vector<ModelNode>& out) {
        std::vector<PendingNode> pending;
        pending.push_back(PendingNode{scene.mRootNode, MODEL_NO_PARENT});
        while (!pending.empty()) {
            const PendingNode current = pending.back();
            pending.pop_back();
            const aiNode& source = *current.node;
            const std::uint32_t index = static_cast<std::uint32_t>(out.size());

            ModelNode node;
            node.name = current.parent == MODEL_NO_PARENT ? std::string(rootName) : std::string(source.mName.C_Str());
            if (node.name.empty()) {
                node.name = std::string(UNNAMED_NODE);
            }
            node.parent = current.parent;
            node.local = bakeLocal(source, meshImporter);

            std::vector<const SourceMesh*> nodeMeshes;
            for (unsigned int i = 0; i < source.mNumMeshes; i++) {
                const SourceMesh& mesh = meshes[source.mMeshes[i]];
                if (mesh.mesh.id.valid()) {
                    nodeMeshes.push_back(&mesh);
                }
            }
            if (nodeMeshes.size() == 1) {
                node.mesh = nodeMeshes[0]->mesh.id;
            }
            out.push_back(std::move(node));

            if (nodeMeshes.size() > 1) {
                for (const SourceMesh* mesh: nodeMeshes) {
                    ModelNode child;
                    child.name = mesh->name;
                    child.parent = index;
                    child.mesh = mesh->mesh.id;
                    out.push_back(std::move(child));
                }
            }

            // Pushed last to first, so the stack pops the children in the source's order.
            for (unsigned int i = source.mNumChildren; i > 0; i--) {
                pending.push_back(PendingNode{source.mChildren[i - 1], index});
            }
        }
    }

    // Walks the nodes in the same order as buildNodes. A mesh no node uses is left out, because it has no world transform.
    void ModelImporter::readMergedParts(const aiScene& scene, const MeshImporter& meshImporter, std::vector<MergedPart>& out) {
        std::vector<PendingMergedNode> pending;
        pending.push_back(PendingMergedNode{scene.mRootNode, Mat4(1.0f)});
        while (!pending.empty()) {
            const PendingMergedNode current = pending.back();
            pending.pop_back();
            const aiNode& source = *current.node;
            const Mat4 world = current.parentWorld * toMat4(source.mTransformation);

            for (unsigned int i = 0; i < source.mNumMeshes; i++) {
                const aiMesh& mesh = *scene.mMeshes[source.mMeshes[i]];
                MergedPart part;
                if (!meshImporter.read(mesh, world, part.mesh)) {
                    TE_LOGGER_WARN("Import: skipped mesh {0} '{1}' of node '{2}', because it is not made of triangles", source.mMeshes[i], mesh.mName.C_Str(), source.mName.C_Str());
                    continue;
                }
                part.materialIndex = mesh.mMaterialIndex;
                out.push_back(std::move(part));
            }

            for (unsigned int i = source.mNumChildren; i > 0; i--) {
                pending.push_back(PendingMergedNode{source.mChildren[i - 1], world});
            }
        }
    }

    // The merged mesh carries a stream when any part has it, and a part without it takes the default.
    // A material that no part uses gets no slot.
    void ModelImporter::joinParts(const aiScene& scene, const MaterialPlan& materials, std::vector<MergedPart>& parts, Mesh& out) {
        std::ranges::stable_sort(parts, std::ranges::less{}, &MergedPart::materialIndex);
        const bool hasNormals = std::ranges::any_of(parts, [](const MergedPart& part) {
            return !part.mesh.normals.empty();
        });
        const bool hasTangents = std::ranges::any_of(parts, [](const MergedPart& part) {
            return !part.mesh.tangents.empty();
        });
        const bool hasUv0 = std::ranges::any_of(parts, [](const MergedPart& part) {
            return !part.mesh.uv0.empty();
        });
        const auto appendStream = [](auto& stream, const auto& partStream, const std::size_t vertexCount, const auto& missing) {
            if (partStream.empty()) {
                stream.insert(stream.end(), vertexCount, missing);
            } else {
                stream.insert(stream.end(), partStream.begin(), partStream.end());
            }
        };

        unsigned int slotMaterial = 0;
        for (const MergedPart& part: parts) {
            if (out.slots.empty() || part.materialIndex != slotMaterial) {
                slotMaterial = part.materialIndex;
                const std::uint32_t firstIndex = static_cast<std::uint32_t>(out.indices.size());
                out.slots.push_back(MeshSlot{uniqueSlotName(slotName(scene, slotMaterial), out.slots), firstIndex, 0, materials.materialId(slotMaterial)});
            }

            const std::uint32_t firstVertex = static_cast<std::uint32_t>(out.positions.size());
            const std::size_t vertexCount = part.mesh.positions.size();
            out.positions.insert(out.positions.end(), part.mesh.positions.begin(), part.mesh.positions.end());
            if (hasNormals) {
                appendStream(out.normals, part.mesh.normals, vertexCount, MERGED_MISSING_NORMAL);
            }
            if (hasTangents) {
                appendStream(out.tangents, part.mesh.tangents, vertexCount, MERGED_MISSING_TANGENT);
            }
            if (hasUv0) {
                appendStream(out.uv0, part.mesh.uv0, vertexCount, MERGED_MISSING_UV0);
            }
            for (const std::uint32_t index: part.mesh.indices) {
                out.indices.push_back(firstVertex + index);
            }
            out.slots.back().indexCount += static_cast<std::uint32_t>(part.mesh.indices.size());
        }
    }

    // The settings turn and scale the whole source, so a node's local transform is carried into the turned space:
    // its translation is turned and scaled, its rotation axis is turned, and its scale follows the axes to their new names.
    TransformValues ModelImporter::bakeLocal(const aiNode& node, const MeshImporter& meshImporter) {
        aiVector3D scaling;
        aiQuaternion rotation;
        aiVector3D position;
        node.mTransformation.Decompose(scaling, rotation, position);

        TransformValues local;
        local.position = meshImporter.bakePosition(Vec3(position.x, position.y, position.z));
        const Vec3 axis = meshImporter.turnUpright(Vec3(rotation.x, rotation.y, rotation.z));
        const Quat turned = Quat(rotation.w, axis.x, axis.y, axis.z);
        if (glm::dot(turned, turned) > MIN_ROTATION_LENGTH_SQUARED) {
            local.rotation = glm::normalize(turned);
        }
        local.scale = Vec3(scaling.x, scaling.y, scaling.z);
        if (meshImporter.getSettings().upAxis == UpAxis::Z) {
            local.scale = Vec3(scaling.x, scaling.z, scaling.y);
        }
        return local;
    }

    std::string ModelImporter::slotName(const aiScene& scene, const unsigned int materialIndex) {
        if (materialIndex < scene.mNumMaterials) {
            const aiString name = scene.mMaterials[materialIndex]->GetName();
            if (name.length > 0) {
                return name.C_Str();
            }
        }
        return std::string(UNNAMED_SLOT) + std::to_string(materialIndex);
    }

    // A renderer's slot list is rebuilt by slot name, so two slots with one name would end up with one material.
    std::string ModelImporter::uniqueSlotName(const std::string& name, const std::vector<MeshSlot>& slots) {
        std::string candidate = name;
        std::uint32_t suffix = 0;
        while (std::ranges::find(slots, candidate, &MeshSlot::name) != slots.end()) {
            suffix++;
            candidate = name + "_" + std::to_string(suffix);
        }
        return candidate;
    }

    char ModelImporter::toLowerAscii(const char character) {
        if (character >= 'A' && character <= 'Z') {
            return static_cast<char>(character - 'A' + 'a');
        }
        return character;
    }

    // Keeps ASCII letters, digits, spaces and . _ - only, so the name is a legal file name on every platform.
    std::string ModelImporter::toFileName(const std::string_view name) {
        std::string fileName;
        fileName.reserve(name.size());
        for (const char character: name) {
            const bool letter = (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
            const bool digit = character >= '0' && character <= '9';
            const bool kept = letter || digit || character == ' ' || character == '.' || character == '_' || character == '-';
            fileName.push_back(kept ? character : '_');
        }

        // Windows drops a trailing dot or space from a file name, which would make two names one file.
        while (!fileName.empty() && (fileName.back() == '.' || fileName.back() == ' ')) {
            fileName.pop_back();
        }
        const std::size_t start = fileName.find_first_not_of(' ');
        if (start == std::string::npos) {
            return std::string(UNNAMED_MESH);
        }
        return fileName.substr(start);
    }

    // Compared without case, because two names that differ only in case are one file on Windows.
    std::string ModelImporter::uniqueFileName(const std::string_view name, std::vector<std::string>& taken) {
        const std::string base = toFileName(name);
        std::string candidate = base;
        std::string key;
        std::uint32_t suffix = 0;
        while (true) {
            key = candidate;
            std::ranges::transform(key, key.begin(), toLowerAscii);
            if (std::ranges::find(taken, key) == taken.end()) {
                break;
            }
            suffix++;
            candidate = base + "_" + std::to_string(suffix);
        }
        taken.push_back(std::move(key));
        return candidate;
    }

    std::string_view ModelImporter::fileStem(const std::string_view folder) {
        const std::size_t slash = folder.find_last_of('/');
        if (slash == std::string_view::npos) {
            return folder;
        }
        return folder.substr(slash + 1);
    }

    bool ModelImporter::validImportScale(const float scale) {
        return std::isfinite(scale) && scale > 0.0f;
    }

    bool ModelImporter::isGltfSource(const std::string_view sourcePath) {
        const std::size_t slash = sourcePath.find_last_of('/');
        const std::size_t dot = sourcePath.find_last_of('.');
        if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash)) {
            return false;
        }
        std::string extension(sourcePath.substr(dot));
        std::ranges::transform(extension, extension.begin(), toLowerAscii);
        return extension == GLTF_EXTENSION || extension == GLB_EXTENSION;
    }

    std::uint32_t ModelImporter::readLittleEndianU32(const std::span<const std::byte> bytes, const std::size_t offset) {
        std::uint32_t value = 0;
        for (std::size_t i = 0; i < 4; i++) {
            value |= static_cast<std::uint32_t>(bytes[offset + i]) << (8 * i);
        }
        return value;
    }

    bool ModelImporter::gltfJson(const std::span<const std::byte> bytes, std::string_view& out) {
        const std::string_view text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        if (!text.starts_with(GLB_MAGIC)) {
            out = text;
            return true;
        }
        if (bytes.size() < GLB_JSON_OFFSET) {
            return false;
        }

        const std::uint32_t chunkLength = readLittleEndianU32(bytes, GLB_JSON_OFFSET - 8);
        const std::uint32_t chunkType = readLittleEndianU32(bytes, GLB_JSON_OFFSET - 4);
        if (chunkType != GLB_JSON_CHUNK_TYPE || chunkLength > bytes.size() - GLB_JSON_OFFSET) {
            return false;
        }
        out = text.substr(GLB_JSON_OFFSET, chunkLength);
        return true;
    }

    std::size_t ModelImporter::skipJsonWhitespace(const std::string_view text, std::size_t position) {
        while (position < text.size() && (text[position] == ' ' || text[position] == '\t' || text[position] == '\n' || text[position] == '\r')) {
            position++;
        }
        return position;
    }

    // position is the string's opening quote. Returns the position after its closing quote, or npos.
    std::size_t ModelImporter::readJsonString(const std::string_view text, const std::size_t position, std::string_view& out) {
        const std::size_t start = position + 1;
        std::size_t current = start;
        while (current < text.size()) {
            if (text[current] == '\\') {
                current += 2;
                continue;
            }
            if (text[current] == '"') {
                out = text.substr(start, current - start);
                return current + 1;
            }
            current++;
        }
        return std::string_view::npos;
    }

    // position is just after the array's opening bracket.
    bool ModelImporter::arrayListsRefusedExtension(const std::string_view text, std::size_t position) {
        while (true) {
            position = skipJsonWhitespace(text, position);
            if (position >= text.size() || text[position] != '"') {
                return false;
            }
            std::string_view name;
            position = readJsonString(text, position, name);
            if (position == std::string_view::npos) {
                return false;
            }
            if (std::ranges::find(REFUSED_GLTF_EXTENSIONS, name) != REFUSED_GLTF_EXTENSIONS.end()) {
                return true;
            }
            position = skipJsonWhitespace(text, position);
            if (position >= text.size() || text[position] != ',') {
                return false;
            }
            position++;
        }
    }

    // Only a key counts: the same text as a string value, a node's name say, is not followed by a colon and an array.
    bool ModelImporter::listsRefusedExtension(const std::string_view json) {
        std::size_t found = json.find(EXTENSIONS_USED_KEY);
        while (found != std::string_view::npos) {
            const std::size_t colon = skipJsonWhitespace(json, found + EXTENSIONS_USED_KEY.size());
            if (colon < json.size() && json[colon] == ':') {
                const std::size_t bracket = skipJsonWhitespace(json, colon + 1);
                if (bracket < json.size() && json[bracket] == '[' && arrayListsRefusedExtension(json, bracket + 1)) {
                    return true;
                }
            }
            found = json.find(EXTENSIONS_USED_KEY, found + 1);
        }
        return false;
    }
}
