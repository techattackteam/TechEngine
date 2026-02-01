#include "ResourcesManager.hpp"


#include "files/FileUtils.hpp"
#include "project/Project.hpp"
#include "systems/SystemsRegistry.hpp"
#include "TechEngine/core/scene/Scene.hpp"

#include <algorithm>

namespace TechEngine {
    ResourcesManager::ResourcesManager(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry),
                                                                           m_meshManager(systemsRegistry),
                                                                           m_materialManager(systemsRegistry),
                                                                           m_textureManager(systemsRegistry) {
    }

    void ResourcesManager::init(AppType appType) {
        std::unordered_map<std::string, std::vector<std::filesystem::path>> filesByExtension = getFilesByExtension(appType);

        std::vector<std::filesystem::path> texturePaths;
        if (filesByExtension.find(".png") != filesByExtension.end()) {
            texturePaths.insert(texturePaths.end(), filesByExtension[".png"].begin(), filesByExtension[".png"].end());
        }
        if (filesByExtension.find(".jpg") != filesByExtension.end()) {
            texturePaths.insert(texturePaths.end(), filesByExtension[".jpg"].begin(), filesByExtension[".jpg"].end());
        }
        if (filesByExtension.find(".hdr") != filesByExtension.end()) {
            texturePaths.insert(texturePaths.end(), filesByExtension[".hdr"].begin(), filesByExtension[".hdr"].end());
        }
        m_textureManager.init(texturePaths);
        m_materialManager.init(filesByExtension[".mat"]);
        m_meshManager.init(filesByExtension[".tesmesh"]);
        assignTextureToMaterial("FlooringBrown", "laminate-flooring-brown_albedo", "albedo");
        assignTextureToMaterial("FlooringBrown", "laminate-flooring-brown_normal-ogl", "normal");
        assignTextureToMaterial("FlooringBrown", "laminate-flooring-brown_metallic", "metallic");
        assignTextureToMaterial("FlooringBrown", "laminate-flooring-brown_roughness", "roughness");
        assignTextureToMaterial("FlooringBrown", "laminate-flooring-brown_ao", "ambient occlusion");

        assignTextureToMaterial("PeelingPaintedMetal", "peeling-painted-metal_albedo", "albedo");
        assignTextureToMaterial("PeelingPaintedMetal", "peeling-painted-metal_normal-ogl", "normal");
        assignTextureToMaterial("PeelingPaintedMetal", "peeling-painted-metal_metallic", "metallic");
        assignTextureToMaterial("PeelingPaintedMetal", "peeling-painted-metal_roughness", "roughness");
        assignTextureToMaterial("PeelingPaintedMetal", "peeling-painted-metal_ao", "ambient occlusion");

        assignTextureToMaterial("Gold", "hammered-gold_albedo", "albedo");
        assignTextureToMaterial("Gold", "hammered-gold_normal-ogl", "normal");
        assignTextureToMaterial("Gold", "hammered-gold_metallic", "metallic");
        assignTextureToMaterial("Gold", "hammered-gold_roughness", "roughness");
        assignTextureToMaterial("Gold", "hammered-gold_ao", "ambient occlusion");

        assignTextureToMaterial("Titanium", "Titanium-Scuffed_baseColor", "albedo");
        assignTextureToMaterial("Titanium", "Titanium-Scuffed_normal", "normal");
        assignTextureToMaterial("Titanium", "Titanium-Scuffed_metallic", "metallic");
        assignTextureToMaterial("Titanium", "Titanium-Scuffed_roughness", "roughness");

        assignTextureToMaterial("Gravel", "gravel_albedo", "albedo");
        assignTextureToMaterial("Gravel", "gravel_normal-ogl", "normal");
        assignTextureToMaterial("Gravel", "gravel_metallic", "metallic");
        assignTextureToMaterial("Gravel", "gravel_roughness", "roughness");
        assignTextureToMaterial("Gravel", "gravel_ao", "ambient occlusion");
    }

    void ResourcesManager::shutdown() {
        m_materialManager.shutdown();
        m_meshManager.shutdown();
    }

#pragma region MeshManager
    void ResourcesManager::createMesh(const std::string& name) {
        m_meshManager.createMesh(name);
    }

    void ResourcesManager::createMeshFromModelFile(const std::string& path) {
        std::string modelName = FileUtils::getFileName(path);
        std::string parentFolder = std::filesystem::path(path).parent_path().string();
        std::string staticMeshPath = parentFolder + "\\" + modelName + ".tesmesh";

        AssimpLoader::ModelData modelData = m_assimpLoader.loadModel(path);
        m_assimpLoader.createStaticMeshFile(modelData.rootNode, staticMeshPath);

        TE_LOGGER_INFO("Created mesh file: {}", staticMeshPath);
    }

    void ResourcesManager::createModelFromFile(const std::string& path) {
        std::string modelName = FileUtils::getFileName(path);
        std::string parentFolder = std::filesystem::path(path).parent_path().string();

        AssimpLoader::ModelData modelData = m_assimpLoader.loadModel(path);
        m_assimpLoader.createModelFiles(modelData, modelName, parentFolder);

        TE_LOGGER_INFO("Created model folder: {}", parentFolder);
    }

    void ResourcesManager::loadModel(const std::string& path, Scene& scene) {
        AssimpLoader::TEModelData teModelData = m_assimpLoader.loadTEModel(path);

        std::string modelFolder = std::filesystem::path(path).parent_path().string();

        // Load all materials from the model
        loadMaterialsFromTEModel(teModelData, modelFolder);

        // Load all meshes
        loadMeshesFromModelNode(teModelData.rootNode, modelFolder);

        // Create entity hierarchy with materials assigned
        createEntityFromModelNode(scene, teModelData, teModelData.rootNode, static_cast<Entity>(-1), modelFolder);

        TE_LOGGER_INFO("Loaded model: {} into scene with {} materials", teModelData.modelName, teModelData.materials.size());
    }

    void ResourcesManager::loadStaticMesh(const std::string& path) {
        std::string modelName = FileUtils::getFileName(path);
        if (m_meshManager.isMeshLoaded(modelName)) {
            TE_LOGGER_WARN("Mesh already registered: {0}", path);
            return;
        }
        m_meshManager.loadStaticMesh(path);
    }

    void ResourcesManager::setVertices(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<int>& indices) {
        if (name == MeshManager::DEFAULT_MESH_NAME) {
            TE_LOGGER_WARN("Cannot set vertices for default mesh");
            return;
        }
        Mesh& mesh = m_meshManager.getMesh(name);
        mesh.setVertices(vertices, indices);
    }

    void ResourcesManager::addVertices(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<int>& indices) {
        if (name == MeshManager::DEFAULT_MESH_NAME) {
            TE_LOGGER_WARN("Cannot add vertices for default mesh");
            return;
        }
        Mesh& mesh = m_meshManager.getMesh(name);
        mesh.addVertices(vertices, indices);
    }

    void ResourcesManager::deleteMesh(const std::string& name) {
        if (name == MeshManager::DEFAULT_MESH_NAME) {
            TE_LOGGER_WARN("Cannot delete default mesh");
            return;
        }
        m_meshManager.deleteMesh(name);
    }

    Mesh& ResourcesManager::getMesh(const std::string& name) {
        if (!m_meshManager.isMeshLoaded(name)) {
            TE_LOGGER_WARN("Mesh not found: {0}", name);
            return getDefaultMesh();
        }
        return m_meshManager.getMesh(name);
    }

    Mesh& ResourcesManager::getDefaultMesh() {
        return m_meshManager.getMesh(MeshManager::DEFAULT_MESH_NAME);
    }

    const std::vector<Mesh>& ResourcesManager::getAllMeshes() {
        return m_meshManager.getMeshes();
    }
#pragma endregion

#pragma region MaterialManager

    Material& ResourcesManager::createMaterial(const std::string& name) {
        return m_materialManager.createMaterial(name);
    }

    void ResourcesManager::loadMaterial(const std::string& name, const std::string& path) {
        if (m_materialManager.materialExists(name)) {
            TE_LOGGER_WARN("Material already registered: {0}", name);
            return;
        }
        m_materialManager.createMaterialFile(name, path);
    }

    Material& ResourcesManager::getMaterial(const std::string& name) {
        if (!m_materialManager.materialExists(name)) {
            TE_LOGGER_WARN("Material not found: {0}", name);
            return getDefaultMaterial();
        }
        return m_materialManager.getMaterial(name);
    }

    Material& ResourcesManager::getDefaultMaterial() {
        return m_materialManager.getMaterial(MaterialManager::DEFAULT_MATERIAL_NAME);
    }

    void ResourcesManager::assignTextureToMaterial(const std::string& materialName, const std::string& textureName, const std::string& textureType) {
        if (!m_materialManager.materialExists(materialName)) {
            TE_LOGGER_WARN("Material not found: {0}", materialName);
            return;
        }
        if (!m_textureManager.textureExists(textureName)) {
            TE_LOGGER_WARN("Texture not found: {0}", textureName);
            return;
        }
        Material& material = m_materialManager.getMaterial(materialName);
        TextureResource& texture = m_textureManager.getTexture(textureName);
        if (textureType == "albedo") {
            material.getAlbedoMapID() = texture.getID();
        } else if (textureType == "normal") {
            material.getNormalMapID() = texture.getID();
        } else if (textureType == "metallic") {
            material.getMetallicMapID() = texture.getID();
        } else if (textureType == "roughness") {
            material.getRoughnessMapID() = texture.getID();
        } else if (textureType == "ambient occlusion") {
            material.getAmbientOcclusionMapID() = texture.getID();
        } else {
            TE_LOGGER_WARN("Unknown texture type: {0}", textureType);
        }
    }

    std::vector<Material*> ResourcesManager::getAllMaterials() {
        return m_materialManager.getMaterials();
    }

#pragma endregion

#pragma region TextureManager
    void ResourcesManager::loadTexture(const std::string& name, const std::string& path) {
        if (m_textureManager.textureExists(name)) {
            TE_LOGGER_WARN("Texture already registered: {0}", name);
            return;
        }
        m_textureManager.loadFromFile(name, path);
    }

    TextureResource& ResourcesManager::getTexture(const std::string& name) {
        if (!m_textureManager.textureExists(name)) {
            TE_LOGGER_WARN("Texture not found: {0}", name);
            throw std::runtime_error("Texture not found");
        }
        return m_textureManager.getTexture(name);
    }

    TextureResource& ResourcesManager::getTexture(const int id) {
        return m_textureManager.getTexture(id);
    }

    std::vector<TextureResource*> ResourcesManager::getAllTextures() {
        return m_textureManager.getTextures();
    }

    std::vector<TextureResource*> ResourcesManager::getAllTexturesOfType(const TextureType& type) {
        return m_textureManager.getTexturesOfType(type);
    }
#pragma endregion

    std::unordered_map<std::string, std::vector<std::filesystem::path>> ResourcesManager::getFilesByExtension(const AppType& appType) {
        std::unordered_map<std::string, std::vector<std::filesystem::path>> filesByExtension;
        std::vector<std::filesystem::path> directories = {
            m_systemsRegistry.getSystem<Project>().getPath(PathType::Assets, AppType::Common).string(),
            m_systemsRegistry.getSystem<Project>().getPath(PathType::Assets, appType).string(),
            m_systemsRegistry.getSystem<Project>().getPath(PathType::Resources, AppType::Common).string(),
            m_systemsRegistry.getSystem<Project>().getPath(PathType::Resources, appType).string(),
            m_systemsRegistry.getSystem<Project>().getPath(PathType::Cache, AppType::Common).string(),
            m_systemsRegistry.getSystem<Project>().getPath(PathType::Cache, appType).string(),
        };

        for (const auto& directory: directories) {
            for (const auto& entry: std::filesystem::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    std::string extension = entry.path().extension().string();
                    if (filesByExtension.find(extension) == filesByExtension.end()) {
                        filesByExtension[extension] = std::vector<std::filesystem::path>();
                    }
                    filesByExtension[extension].emplace_back(entry.path());
                }
            }
        }
        return filesByExtension;
    }

    void ResourcesManager::registerMeshesFromNode(const AssimpLoader::Node& node, const std::string& modelName, const std::string& parentPath) {
        std::string nodePath = parentPath.empty() ? node.name : parentPath + "/" + node.name;
        if (nodePath.empty()) {
            nodePath = modelName;
        }

        for (size_t i = 0; i < node.meshes.size(); ++i) {
            std::string meshName = modelName + "_" + nodePath + "_mesh" + std::to_string(i);
            std::replace(meshName.begin(), meshName.end(), '/', '_');
            std::replace(meshName.begin(), meshName.end(), '\\', '_');

            if (!m_meshManager.isMeshLoaded(meshName)) {
                m_meshManager.createMesh(meshName, node.meshes[i].vertices, node.meshes[i].indices);
            }
        }

        for (const AssimpLoader::Node& child: node.children) {
            registerMeshesFromNode(child, modelName, nodePath);
        }
    }

    Entity ResourcesManager::createEntityFromNode(Scene& scene, const AssimpLoader::Node& node, Entity parent, const std::string& modelName, const std::string& nodePath) {
        std::string entityName = node.name.empty() ? modelName : node.name;

        std::string currentNodePath = nodePath.empty() ? node.name : nodePath + "/" + node.name;
        if (currentNodePath.empty()) {
            currentNodePath = modelName;
        }

        Entity entity = scene.createEntity(entityName);

        Transform& transform = scene.getComponent<Transform>(entity);
        transform.m_position = node.position;
        transform.m_rotation = node.rotation;
        transform.m_scale = node.scale;
        transform.calculateUpForwardRight();
        transform.m_isDirty = true;

        if (parent != static_cast<Entity>(-1)) {
            scene.setParent(parent, entity);
        }

        for (size_t i = 0; i < node.meshes.size(); ++i) {
            std::string meshName = modelName + "_" + currentNodePath + "_mesh" + std::to_string(i);
            std::replace(meshName.begin(), meshName.end(), '/', '_');
            std::replace(meshName.begin(), meshName.end(), '\\', '_');

            std::string meshEntityName = entityName + "_mesh" + std::to_string(i);
            Entity meshEntity = scene.createEntity(meshEntityName);

            Transform& meshTransform = scene.getComponent<Transform>(meshEntity);
            meshTransform.m_position = glm::vec3(0.0f);
            meshTransform.m_rotation = glm::vec3(0.0f);
            meshTransform.m_scale = glm::vec3(1.0f);
            meshTransform.m_isDirty = true;

            scene.setParent(entity, meshEntity);

            MeshRenderer meshRenderer;
            meshRenderer.mesh = &getMesh(meshName);
            meshRenderer.material = &getDefaultMaterial();
            scene.addComponent(meshEntity, meshRenderer);
        }

        for (const AssimpLoader::Node& child: node.children) {
            createEntityFromNode(scene, child, entity, modelName, currentNodePath);
        }

        return entity;
    }

    void ResourcesManager::loadMeshesFromModelNode(const AssimpLoader::ModelNode& node, const std::string& modelFolder) {
        // Load all mesh files referenced by this node
        for (const std::string& meshFile: node.meshFiles) {
            std::string meshPath = modelFolder + "\\" + meshFile;
            std::string meshName = std::filesystem::path(meshFile).stem().string();

            if (!m_meshManager.isMeshLoaded(meshName)) {
                m_meshManager.loadStaticMesh(meshPath);
            }
        }

        // Recursively load meshes from children
        for (const AssimpLoader::ModelNode& child: node.children) {
            loadMeshesFromModelNode(child, modelFolder);
        }
    }

    void ResourcesManager::loadMaterialsFromTEModel(const AssimpLoader::TEModelData& teModelData, const std::string& modelFolder) {
        for (const AssimpLoader::MaterialData& matData : teModelData.materials) {
            // Skip if material already exists
            if (m_materialManager.materialExists(matData.name)) {
                TE_LOGGER_INFO("Material '{}' already exists, skipping", matData.name);
                continue;
            }

            // Create material with the albedo color
            Material& material = m_materialManager.createMaterial(matData.name, matData.albedoColor);
            material.getProperties().metallic = matData.metallic;
            material.getProperties().roughness = matData.roughness;

            // Helper lambda to load texture and assign to material
            auto loadAndAssignTexture = [&](const std::string& texturePath, int& materialMapID, const std::string& textureType) {
                if (texturePath.empty()) {
                    return;
                }

                // Check if the path exists as-is (absolute path)
                std::string finalPath = texturePath;
                if (!std::filesystem::exists(finalPath)) {
                    // Try relative to model folder
                    finalPath = modelFolder + "\\" + std::filesystem::path(texturePath).filename().string();
                }

                if (!std::filesystem::exists(finalPath)) {
                    TE_LOGGER_WARN("Texture file not found for material '{}': {} (tried: {})", matData.name, texturePath, finalPath);
                    return;
                }

                std::string texName = std::filesystem::path(finalPath).stem().string();
                if (!m_textureManager.textureExists(texName)) {
                    bool loaded = m_textureManager.loadFromFile(texName, finalPath);
                    if (!loaded) {
                        TE_LOGGER_ERROR("Failed to load texture '{}' from '{}'", texName, finalPath);
                        return;
                    }
                    TE_LOGGER_INFO("Loaded {} texture '{}' for material '{}'", textureType, texName, matData.name);
                }
                materialMapID = m_textureManager.getTexture(texName).getID();
            };

            // Load and assign textures
            loadAndAssignTexture(matData.albedoTexture, material.getAlbedoMapID(), "albedo");
            loadAndAssignTexture(matData.normalTexture, material.getNormalMapID(), "normal");
            loadAndAssignTexture(matData.metallicTexture, material.getMetallicMapID(), "metallic");
            loadAndAssignTexture(matData.roughnessTexture, material.getRoughnessMapID(), "roughness");
            loadAndAssignTexture(matData.aoTexture, material.getAmbientOcclusionMapID(), "ao");
            loadAndAssignTexture(matData.emissionTexture, material.getEmissionMapID(), "emission");

            TE_LOGGER_INFO("Created material '{}' from temodel (albedoMapID={}, normalMapID={}, metallicMapID={}, roughnessMapID={})",
                matData.name, material.getAlbedoMapID(), material.getNormalMapID(),
                material.getMetallicMapID(), material.getRoughnessMapID());
        }
    }

    Entity ResourcesManager::createEntityFromModelNode(Scene& scene, const AssimpLoader::TEModelData& teModelData, const AssimpLoader::ModelNode& node, Entity parent, const std::string& modelFolder) {
        std::string entityName = node.name.empty() ? "Node" : node.name;

        Entity entity = scene.createEntity(entityName);

        // Set transform from node data
        Transform& transform = scene.getComponent<Transform>(entity);
        transform.m_position = node.position;
        transform.m_rotation = node.rotation;
        transform.m_scale = node.scale;
        transform.calculateUpForwardRight();
        transform.m_isDirty = true;

        // Set parent if provided
        if (parent != static_cast<Entity>(-1)) {
            scene.setParent(parent, entity);
        }

        // Create a child entity for each mesh (since each entity can only have one MeshRenderer)
        for (size_t i = 0; i < node.meshFiles.size(); ++i) {
            std::string meshFileName = node.meshFiles[i];
            std::string meshName = std::filesystem::path(meshFileName).stem().string();

            std::string meshEntityName = entityName + "_mesh" + std::to_string(i);
            Entity meshEntity = scene.createEntity(meshEntityName);

            // Mesh entities inherit identity transform (relative to parent node)
            Transform& meshTransform = scene.getComponent<Transform>(meshEntity);
            meshTransform.m_position = glm::vec3(0.0f);
            meshTransform.m_rotation = glm::vec3(0.0f);
            meshTransform.m_scale = glm::vec3(1.0f);
            meshTransform.m_isDirty = true;

            scene.setParent(entity, meshEntity);

            MeshRenderer meshRenderer;
            meshRenderer.mesh = &getMesh(meshName);

            // Assign the correct material for this mesh
            if (i < node.materialNames.size() && !node.materialNames[i].empty()) {
                std::string materialName = node.materialNames[i];
                if (m_materialManager.materialExists(materialName)) {
                    meshRenderer.material = &m_materialManager.getMaterial(materialName);
                } else {
                    meshRenderer.material = &getDefaultMaterial();
                }
            } else {
                meshRenderer.material = &getDefaultMaterial();
            }

            scene.addComponent(meshEntity, meshRenderer);
        }

        // Recursively create child entities
        for (const AssimpLoader::ModelNode& child: node.children) {
            createEntityFromModelNode(scene, teModelData, child, entity, modelFolder);
        }

        return entity;
    }
}
