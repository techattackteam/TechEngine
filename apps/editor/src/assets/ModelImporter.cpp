#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/platform/files/VirtualPath.hpp>

#include <assets/MeshImporter.hpp>
#include <assets/ModelImporter.hpp>
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
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    static constexpr std::string_view MESH_FOLDER = "Meshes";
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
    static constexpr float MIN_ROTATION_LENGTH_SQUARED = 1.0e-12f;

    struct ModelImporter::SourceMesh {
        // A null id marks a source mesh that is not written, so no node refers to it.
        Mesh mesh;
        std::string name;
        std::string fileName;
    };

    struct PendingNode {
        const aiNode* node = nullptr;
        std::uint32_t parent = MODEL_NO_PARENT;
    };

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

    std::error_code ModelImporter::importFile(const ModelImportRequest& request, ModelImportResult& out) const {
        TE_PROFILER_FUNCTION();
        if (!validImportScale(request.settings.scale)) {
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
        if (!folderError) {
            return ProjectError::ImportFolderExists;
        }
        if (folderError != FileError::NotFound) {
            return folderError;
        }

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
        const aiScene* scene = importer.ReadFile(assimpPath, IMPORT_STEPS);
        if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || scene->mRootNode == nullptr) {
            TE_LOGGER_WARN("Import: assimp did not read {0}: {1}", request.sourcePath, importer.GetErrorString());
            return ProjectError::SourceUnreadable;
        }

        const MeshImporter meshImporter(request.settings, gltfSource);
        std::vector<SourceMesh> meshes;
        const std::uint32_t meshCount = readMeshes(*scene, parts.relative, meshImporter, meshes);
        if (meshCount == 0) {
            return ProjectError::SourceHasNoMeshes;
        }

        Model model;
        model.id = m_generateId();
        model.importBlock.sourcePath = std::string(parts.relative);
        model.importBlock.settings = request.settings;
        buildNodes(*scene, meshes, fileStem(folder), meshImporter, model.nodes);

        const std::error_code createError = m_files->createDirectory(folder);
        if (createError == FileError::AlreadyExists) {
            return ProjectError::ImportFolderExists;
        }
        if (createError) {
            return createError;
        }

        std::vector<AssetRecord> records;
        records.reserve(static_cast<std::size_t>(meshCount) + 1);
        const std::error_code writeError = writeOutputs(model, meshes, request.side, folder, records);
        if (writeError) {
            // A folder left behind would refuse every later import of this source.
            const std::error_code removeError = m_files->remove(folder, true);
            if (removeError) {
                TE_LOGGER_WARN("Import: failed to remove the unfinished {0}: {1}", folder, removeError.message());
            }
            return writeError;
        }

        out.folder = folder;
        out.nodeCount = static_cast<std::uint32_t>(model.nodes.size());
        out.records = std::move(records);
        return {};
    }

    std::uint32_t ModelImporter::readMeshes(const aiScene& scene, const std::string_view sourcePath, const MeshImporter& meshImporter, std::vector<SourceMesh>& out) const {
        std::vector<std::string> takenFileNames;
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
            mesh.mesh.slots = {MeshSlot{slotName(scene, source), 0, static_cast<std::uint32_t>(mesh.mesh.indices.size()), ResourceId{}}};
            mesh.name = source.mName.C_Str();
            mesh.fileName = uniqueFileName(mesh.name, takenFileNames);
            if (mesh.name.empty()) {
                mesh.name = mesh.fileName;
            }
            written++;
        }
        return written;
    }

    std::error_code ModelImporter::writeOutputs(const Model& model, const std::vector<SourceMesh>& meshes, const Side side, const std::string& folder, std::vector<AssetRecord>& records) const {
        const std::string meshFolder = folder + "/" + std::string(MESH_FOLDER);
        const std::error_code meshFolderError = m_files->createDirectory(meshFolder);
        if (meshFolderError) {
            return meshFolderError;
        }

        std::vector<std::byte> bytes;
        for (const SourceMesh& mesh: meshes) {
            if (!mesh.mesh.id.valid()) {
                continue;
            }
            bytes.clear();
            mesh.mesh.write(bytes);
            std::string path = meshFolder + "/" + mesh.fileName + std::string(MESH_EXTENSION);
            const std::error_code meshError = m_files->write(path, bytes);
            if (meshError) {
                return meshError;
            }
            records.push_back(AssetRecord{mesh.mesh.id, MESH_TYPE_TAG, side, std::move(path)});
        }

        // The model is written last, so a folder that holds a model holds every mesh the model names.
        bytes.clear();
        model.write(bytes);
        std::string modelPath = folder + "/" + std::string(fileStem(folder)) + std::string(MODEL_EXTENSION);
        const std::error_code modelError = m_files->write(modelPath, bytes);
        if (modelError) {
            return modelError;
        }
        records.insert(records.begin(), AssetRecord{model.id, MODEL_TYPE_TAG, side, std::move(modelPath)});
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

    std::string ModelImporter::slotName(const aiScene& scene, const aiMesh& source) {
        if (source.mMaterialIndex < scene.mNumMaterials) {
            const aiString name = scene.mMaterials[source.mMaterialIndex]->GetName();
            if (name.length > 0) {
                return name.C_Str();
            }
        }
        return std::string(UNNAMED_SLOT) + std::to_string(source.mMaterialIndex);
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
