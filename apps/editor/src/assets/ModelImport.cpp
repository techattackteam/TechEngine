#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/platform/files/VirtualPath.hpp>

#include <assets/ModelImport.hpp>
#include <project/ProjectError.hpp>

#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
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
    static constexpr unsigned int IMPORT_STEPS = aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_JoinIdenticalVertices;
    static constexpr unsigned int NOT_TRIANGLES = aiPrimitiveType_POINT | aiPrimitiveType_LINE | aiPrimitiveType_POLYGON;
    static constexpr float MIN_ROTATION_LENGTH_SQUARED = 1.0e-12f;

    struct SourceMesh {
        // A null id marks a source mesh that is not written, so no node refers to it.
        Mesh mesh;
        std::string name;
        std::string fileName;
    };

    struct PendingNode {
        const aiNode* node = nullptr;
        std::uint32_t parent = MODEL_NO_PARENT;
    };

    // A Z-up source turns by -90 degrees about X. That is a rotation, so no triangle changes its winding.
    static Vec3 turnUpright(const Vec3 value, const UpAxis upAxis) {
        if (upAxis == UpAxis::Z) {
            return Vec3(value.x, value.z, -value.y);
        }
        return value;
    }

    static Vec3 bakePosition(const aiVector3D& position, const ImportSettings& settings) {
        return turnUpright(Vec3(position.x, position.y, position.z), settings.upAxis) * settings.scale;
    }

    // The settings turn and scale the whole source, so a node's local transform is carried into the turned space:
    // its translation is turned and scaled, its rotation axis is turned, and its scale follows the axes to their new names.
    static TransformValues bakeLocal(const aiMatrix4x4& transformation, const ImportSettings& settings) {
        aiVector3D scaling;
        aiQuaternion rotation;
        aiVector3D position;
        transformation.Decompose(scaling, rotation, position);

        TransformValues local;
        local.position = bakePosition(position, settings);
        const Vec3 axis = turnUpright(Vec3(rotation.x, rotation.y, rotation.z), settings.upAxis);
        const Quat turned = Quat(rotation.w, axis.x, axis.y, axis.z);
        if (glm::dot(turned, turned) > MIN_ROTATION_LENGTH_SQUARED) {
            local.rotation = glm::normalize(turned);
        }
        local.scale = Vec3(scaling.x, scaling.y, scaling.z);
        if (settings.upAxis == UpAxis::Z) {
            local.scale = Vec3(scaling.x, scaling.z, scaling.y);
        }
        return local;
    }

    static bool readMesh(const aiMesh& source, const ImportSettings& settings, Mesh& out) {
        const bool trianglesOnly = (source.mPrimitiveTypes & aiPrimitiveType_TRIANGLE) != 0 && (source.mPrimitiveTypes & NOT_TRIANGLES) == 0;
        if (!trianglesOnly || source.mNumVertices == 0 || source.mNumFaces == 0) {
            return false;
        }

        std::vector<std::uint32_t> indices;
        indices.reserve(static_cast<std::size_t>(source.mNumFaces) * 3);
        for (unsigned int i = 0; i < source.mNumFaces; i++) {
            const aiFace& face = source.mFaces[i];
            if (face.mNumIndices != 3) {
                return false;
            }
            for (unsigned int corner = 0; corner < 3; corner++) {
                if (face.mIndices[corner] >= source.mNumVertices) {
                    return false;
                }
                indices.push_back(face.mIndices[corner]);
            }
        }

        std::vector<Vec3> positions;
        positions.reserve(source.mNumVertices);
        for (unsigned int i = 0; i < source.mNumVertices; i++) {
            positions.push_back(bakePosition(source.mVertices[i], settings));
        }

        out.positions = std::move(positions);
        out.indices = std::move(indices);
        return true;
    }

    static char toLowerAscii(const char character) {
        if (character >= 'A' && character <= 'Z') {
            return static_cast<char>(character - 'A' + 'a');
        }
        return character;
    }

    // Keeps ASCII letters, digits, spaces and . _ - only, so the name is a legal file name on every platform.
    static std::string toFileName(const std::string_view name) {
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
    static std::string uniqueFileName(const std::string_view name, std::vector<std::string>& taken) {
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

    static std::uint32_t readMeshes(const aiScene& scene, const ImportSettings& settings, const std::function<ResourceId()>& generateId, std::vector<SourceMesh>& out) {
        std::vector<std::string> takenFileNames;
        std::uint32_t written = 0;
        out.resize(scene.mNumMeshes);
        for (unsigned int i = 0; i < scene.mNumMeshes; i++) {
            const aiMesh& source = *scene.mMeshes[i];
            SourceMesh& mesh = out[i];
            if (!readMesh(source, settings, mesh.mesh)) {
                TE_LOGGER_WARN("Import: skipped mesh {0} '{1}', because it is not made of triangles", i, source.mName.C_Str());
                continue;
            }
            mesh.mesh.id = generateId();
            mesh.name = source.mName.C_Str();
            mesh.fileName = uniqueFileName(mesh.name, takenFileNames);
            if (mesh.name.empty()) {
                mesh.name = mesh.fileName;
            }
            written++;
        }
        return written;
    }

    static void buildNodes(const aiScene& scene, const std::vector<SourceMesh>& meshes, const std::string_view rootName, const ImportSettings& settings, std::vector<ModelNode>& out) {
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
            node.local = bakeLocal(source.mTransformation, settings);

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

    static std::string_view fileStem(const std::string_view folder) {
        const std::size_t slash = folder.find_last_of('/');
        if (slash == std::string_view::npos) {
            return folder;
        }
        return folder.substr(slash + 1);
    }

    static std::error_code writeOutputs(FileAccess& files, const Model& model, const std::vector<SourceMesh>& meshes, const Side side, const std::string& folder, std::vector<AssetRecord>& records) {
        const std::string meshFolder = folder + "/" + std::string(MESH_FOLDER);
        const std::error_code meshFolderError = files.createDirectory(meshFolder);
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
            const std::error_code meshError = files.write(path, bytes);
            if (meshError) {
                return meshError;
            }
            records.push_back(AssetRecord{mesh.mesh.id, MESH_TYPE_TAG, side, std::move(path)});
        }

        // The model is written last, so a folder that holds a model holds every mesh the model names.
        bytes.clear();
        model.write(bytes);
        std::string modelPath = folder + "/" + std::string(fileStem(folder)) + std::string(MODEL_EXTENSION);
        const std::error_code modelError = files.write(modelPath, bytes);
        if (modelError) {
            return modelError;
        }
        records.insert(records.begin(), AssetRecord{model.id, MODEL_TYPE_TAG, side, std::move(modelPath)});
        return {};
    }

    std::string modelImportFolder(const std::string_view sourcePath) {
        const std::size_t slash = sourcePath.find_last_of('/');
        const std::size_t dot = sourcePath.find_last_of('.');
        const bool hasExtension = dot != std::string_view::npos && (slash == std::string_view::npos || dot > slash + 1);
        if (!hasExtension) {
            return std::string(sourcePath);
        }
        return std::string(sourcePath.substr(0, dot));
    }

    std::error_code importModel(FileAccess& files, const ModelImportRequest& request, const std::function<ResourceId()>& generateId, ModelImportResult& out) {
        TE_PROFILER_FUNCTION();
        VirtualPathParts parts;
        const std::error_code splitError = splitVirtualPath(request.sourcePath, parts);
        if (splitError) {
            return splitError;
        }

        std::filesystem::path physicalPath;
        const std::error_code resolveError = files.resolve(request.sourcePath, physicalPath);
        if (resolveError) {
            return resolveError;
        }

        const std::string folder = modelImportFolder(request.sourcePath);
        FileStatus folderStatus;
        const std::error_code folderError = files.status(folder, folderStatus);
        if (!folderError) {
            return ProjectError::ImportFolderExists;
        }
        if (folderError != FileError::NotFound) {
            return folderError;
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

        std::vector<SourceMesh> meshes;
        const std::uint32_t meshCount = readMeshes(*scene, request.settings, generateId, meshes);
        if (meshCount == 0) {
            return ProjectError::SourceHasNoMeshes;
        }

        Model model;
        model.id = generateId();
        model.importBlock.sourcePath = std::string(parts.relative);
        model.importBlock.settings = request.settings;
        buildNodes(*scene, meshes, fileStem(folder), request.settings, model.nodes);

        const std::error_code createError = files.createDirectory(folder);
        if (createError == FileError::AlreadyExists) {
            return ProjectError::ImportFolderExists;
        }
        if (createError) {
            return createError;
        }

        std::vector<AssetRecord> records;
        records.reserve(static_cast<std::size_t>(meshCount) + 1);
        const std::error_code writeError = writeOutputs(files, model, meshes, request.side, folder, records);
        if (writeError) {
            // A folder left behind would refuse every later import of this source.
            const std::error_code removeError = files.remove(folder, true);
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
}
