#include "GpuResourceManager.hpp"

#include "Material.hpp"
#include "Texture.hpp"

#include "eventSystem/EventManager.hpp"
#include "resources/ResourceSystem.hpp"
#include "systems/SystemsRegistry.hpp"

#include "TechEngine/core/resources/material/MaterialResource.hpp"
#include "events/resourcersManager/materials/MaterialCreatedEvent.hpp"
#include "events/resourcersManager/materials/MaterialDeletedEvent.hpp"
#include "events/resourcersManager/mesh/MeshCreatedEvent.hpp"
#include "events/resourcersManager/mesh/MeshDeletedEvent.hpp"
#include "events/resourcersManager/model/ModelCreatedEvent.hpp"
#include "events/resourcersManager/texture/TextureCreatedEvent.hpp"
#include "events/resourcersManager/texture/TextureDeletedEvent.hpp"
#include "events/resourcersManager/shader/ShaderCreatedEvent.hpp"
#include "events/resourcersManager/shader/ShaderDeletedEvent.hpp"

namespace TechEngine {
    GpuResourceManager::GpuResourceManager(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry),
                                                                               m_materialsCache(8, &m_materialsBuffer),
                                                                               m_meshesCache(8, nullptr),
                                                                               m_texturesCache(8, nullptr),
                                                                               m_shadersCache(8, nullptr) {
        EventManager& eventManager = systemsRegistry.getSystem<EventManager>();
        eventManager.subscribe<MaterialCreatedEvent>([this](const std::shared_ptr<Event>& event) {
            this->uploadNewMaterial(dynamic_cast<const MaterialCreatedEvent*>(event.get())->getUUID());
        });

        eventManager.subscribe<MaterialDeletedEvent>([this](const std::shared_ptr<Event>& event) {
            this->removeMaterial(dynamic_cast<const MaterialDeletedEvent*>(event.get())->getUUID());
        });

        eventManager.subscribe<MeshCreatedEvent>([this](const std::shared_ptr<Event>& event) {
            this->uploadNewMeshResource(dynamic_cast<const MeshCreatedEvent*>(event.get())->getUUID());
        });

        eventManager.subscribe<MeshDeletedEvent>([this](const std::shared_ptr<Event>& event) {
            this->removeMeshResource(dynamic_cast<const MeshDeletedEvent*>(event.get())->getUUID());
        });

        eventManager.subscribe<TextureCreatedEvent>([this](const std::shared_ptr<Event>& event) {
            this->uploadNewTexture(dynamic_cast<const TextureCreatedEvent*>(event.get())->getUUID());
        });

        eventManager.subscribe<TextureDeletedEvent>([this](const std::shared_ptr<Event>& event) {
            const UUID uuid = dynamic_cast<const TextureDeletedEvent*>(event.get())->getUUID();
            this->removeTexture(uuid);
            m_materialsCache.forEach([this](Material& material, uint32_t) {
                this->updateMaterial(material.getUUID());
            });
        });

        eventManager.subscribe<ShaderCreatedEvent>([this](const std::shared_ptr<Event>& event) {
            this->uploadNewShader(dynamic_cast<const ShaderCreatedEvent*>(event.get())->getUUID());
        });

        eventManager.subscribe<ShaderDeletedEvent>([this](const std::shared_ptr<Event>& event) {
            this->removeShader(dynamic_cast<const ShaderDeletedEvent*>(event.get())->getUUID());
        });
    }

    void GpuResourceManager::init() {
        m_meshesVertexArray.init();
        m_meshesVertexBuffer.init(2 * sizeof(Vertex));
        m_meshesIndexBuffer.init(2 * sizeof(uint32_t));
        m_meshesVertexArray.addNewBuffer(m_meshesVertexBuffer);

        m_materialsBuffer.init(8 * sizeof(MaterialProperties), sizeof(MaterialProperties));
    }

    void GpuResourceManager::uploadNewMeshResource(const UUID& meshUUID) {
        const MeshResource* meshResource = m_systemsRegistry.getSystem<ResourceSystem>().getMeshResource(meshUUID).get();
        const std::vector<Vertex>& vertices = meshResource->getVertices();
        const std::vector<int>& indices = meshResource->getIndices();

        m_meshesVertexBuffer.addData(vertices.data(), vertices.size() * sizeof(Vertex), m_currentVertexOffset * sizeof(Vertex));
        m_meshesIndexBuffer.addData(indices.data(), indices.size() * sizeof(uint32_t), m_currentIndexOffset * sizeof(uint32_t));
        m_meshesVertexArray.addNewBuffer(m_meshesVertexBuffer);

        Mesh mesh(meshUUID, indices.size(), vertices.size(), m_currentIndexOffset, m_currentVertexOffset);
        m_meshesCache.add(meshUUID, std::move(mesh));

        m_currentVertexOffset += vertices.size();
        m_currentIndexOffset += indices.size();
    }

    void GpuResourceManager::removeMeshResource(const UUID& meshUUID) {
        Mesh* mesh = m_meshesCache.get(meshUUID);
        if (!mesh) {
            return;
        }

        const uint32_t removedBaseVertex = mesh->getBaseVertex();
        const uint32_t removedVertexCount = mesh->getVertexCount();
        const uint32_t removedFirstIndex = mesh->getFirstIndex();
        const uint32_t removedIndexCount = mesh->getIndexCount();

        m_meshesVertexBuffer.remove(removedBaseVertex * sizeof(Vertex), removedVertexCount * sizeof(Vertex));
        m_meshesCache.forEach([&](Mesh& m, uint32_t) {
            if (m.getBaseVertex() > removedBaseVertex) {
                m.m_baseVertex -= removedVertexCount;
            }
        });

        m_meshesIndexBuffer.remove(removedFirstIndex * sizeof(uint32_t), removedIndexCount * sizeof(uint32_t));
        m_meshesCache.forEach([&](Mesh& m, uint32_t) {
            if (m.getFirstIndex() > removedFirstIndex) {
                m.m_firstIndex -= removedIndexCount;
            }
        });

        m_currentVertexOffset -= removedVertexCount;
        m_currentIndexOffset -= removedIndexCount;

        m_meshesCache.remove(meshUUID);
    }

    const Mesh* GpuResourceManager::getMesh(const UUID& meshUUID) const {
        if (const Mesh* mesh = m_meshesCache.get(meshUUID)) {
            return mesh;
        }
        TE_LOGGER_WARN("Mesh not found with ID: {0}", meshUUID.toString());
        return nullptr;
    }

    void GpuResourceManager::uploadNewMaterial(const UUID& materialUUID) {
        std::shared_ptr<MaterialResource> materialResource = m_systemsRegistry.getSystem<ResourceSystem>().getMaterialResource(materialUUID);
        MaterialProperties properties;

        properties.albedo = materialResource->getAlbedo();
        properties.metallic = materialResource->getMetallic();
        properties.roughness = materialResource->getRoughness();
        properties.ambientOcclusion = materialResource->getAmbientOcclusion();
        properties.emission = materialResource->getEmission();

        if (!materialResource->getAlbedoMapUUID().isNull()) {
            properties.albedoMapHandle = m_texturesCache.get(materialResource->getAlbedoMapUUID())->getHandle();
        }
        if (!materialResource->getNormalMapUUID().isNull()) {
            properties.normalMapHandle = m_texturesCache.get(materialResource->getNormalMapUUID())->getHandle();
        }
        if (!materialResource->getMetallicMapUUID().isNull()) {
            properties.metallicMapHandle = m_texturesCache.get(materialResource->getMetallicMapUUID())->getHandle();
        }
        if (!materialResource->getRoughnessMapUUID().isNull()) {
            if (!m_texturesCache.contains(materialResource->getRoughnessMapUUID())) {
                if (m_systemsRegistry.getSystem<ResourceSystem>().getTextureResource(materialResource->getRoughnessMapUUID())) {
                    uploadNewTexture(materialResource->getRoughnessMapUUID());
                }
            }
            properties.roughnessMapHandle = m_texturesCache.get(materialResource->getRoughnessMapUUID())->getHandle();
        }
        if (!materialResource->getAmbientOcclusionMapUUID().isNull()) {
            properties.ambientOcclusionMapHandle = m_texturesCache.get(materialResource->getAmbientOcclusionMapUUID())->getHandle();
        }
        if (!materialResource->getEmissionMapUUID().isNull()) {
            properties.emissionMapHandle = m_texturesCache.get(materialResource->getEmissionMapUUID())->getHandle();
        }

        Material material(properties, materialResource->getUUID());
        uint32_t slot = m_materialsCache.add(materialUUID, std::move(material));
        m_materialsBuffer.addData(&properties, sizeof(MaterialProperties), slot);
        m_materialsCache.get(materialUUID)->setSSBOSlot(slot);
    }

    void GpuResourceManager::updateMaterial(const UUID& materialUUID) {
        Material* material = m_materialsCache.get(materialUUID);
        if (!material) {
            return;
        }
        const std::shared_ptr<MaterialResource> materialResource = m_systemsRegistry.getSystem<ResourceSystem>().getMaterialResource(materialUUID);
        if (!materialResource) {
            return;
        }

        MaterialProperties properties;
        properties.albedo = materialResource->getAlbedo();
        properties.metallic = materialResource->getMetallic();
        properties.roughness = materialResource->getRoughness();
        properties.ambientOcclusion = materialResource->getAmbientOcclusion();
        properties.emission = materialResource->getEmission();

        const auto handleFor = [this](const UUID& textureUUID) -> uint64_t {
            if (textureUUID.isNull() || !m_texturesCache.contains(textureUUID)) {
                return 0;
            }
            return m_texturesCache.get(textureUUID)->getHandle();
        };

        properties.albedoMapHandle = handleFor(materialResource->getAlbedoMapUUID());
        properties.normalMapHandle = handleFor(materialResource->getNormalMapUUID());
        properties.roughnessMapHandle = handleFor(materialResource->getRoughnessMapUUID());
        properties.ambientOcclusionMapHandle = handleFor(materialResource->getAmbientOcclusionMapUUID());
        properties.emissionMapHandle = handleFor(materialResource->getEmissionMapUUID());

        const uint32_t slot = m_materialsCache.getSlot(materialUUID);
        if (slot != static_cast<uint32_t>(-1)) {
            m_materialsBuffer.addData(&properties, sizeof(MaterialProperties), slot);
        }
    }

    void GpuResourceManager::removeMaterial(const UUID& materialUUID) {
        // Clear the SSBO slot BEFORE removing from the cache: remove() frees the slot, so reading
        // it afterwards would return -1.
        const uint32_t slot = m_materialsCache.getSlot(materialUUID);
        if (slot != static_cast<uint32_t>(-1)) {
            m_materialsBuffer.clearData(slot);
        }
        m_materialsCache.remove(materialUUID);
    }

    const Material* GpuResourceManager::getMaterial(const UUID& materialUUID) const {
        if (const Material* material = m_materialsCache.get(materialUUID)) {
            return material;
        }
        TE_LOGGER_WARN("Material not found with ID: {0}", materialUUID.toString());
        return nullptr;
    }

    void GpuResourceManager::uploadNewTexture(const UUID& textureUUID) {
        std::shared_ptr<TextureResource> textureResource = m_systemsRegistry.getSystem<ResourceSystem>().getTextureResource(textureUUID);

        Texture texture(textureUUID);
        texture.uploadFromResource(*textureResource);
        texture.makeResident();
        m_texturesCache.add(textureUUID, std::move(texture));
    }

    void GpuResourceManager::removeTexture(const UUID& textureUUID) {
        Texture* texture = m_texturesCache.get(textureUUID);
        if (!texture) {
            return;
        }
        texture->makeNonResident();
        texture->deleteTexture();
        m_texturesCache.remove(textureUUID);
    }

    const Texture* GpuResourceManager::getTexture(const UUID& textureUUID) const {
        if (const Texture* texture = m_texturesCache.get(textureUUID)) {
            return texture;
        }
        TE_LOGGER_WARN("Texture not found with ID: {0}", textureUUID.toString());
        return nullptr;
    }

    void GpuResourceManager::uploadNewShader(const UUID& shaderUUID) {
        std::shared_ptr<ShaderResource> shaderResource = m_systemsRegistry.getSystem<ResourceSystem>().getShaderResource(shaderUUID);

        Shader shader(shaderResource->getName(), shaderUUID);
        shader.link(shaderResource->getSources());
        m_shadersCache.add(shaderUUID, std::move(shader));
    }

    void GpuResourceManager::removeShader(const UUID& shaderUUID) {
        m_shadersCache.remove(shaderUUID);
    }

    void GpuResourceManager::changeActiveShader(const std::string& shaderName) {
        if (m_activeShader != nullptr) {
            m_activeShader->unBind();
        }
        m_activeShader = m_shadersCache.get(shaderName);
        if (m_activeShader) {
            m_activeShader->bind();
        }
    }

    const Shader* GpuResourceManager::getShader(const UUID& shaderUUID) const {
        if (const Shader* shader = m_shadersCache.get(shaderUUID)) {
            return shader;
        }
        TE_LOGGER_WARN("Shader not found with ID: {0}", shaderUUID.toString());
        return nullptr;
    }

    const Shader* GpuResourceManager::getShader(const std::string& shaderName) const {
        const Shader* shader = m_shadersCache.get(shaderName);
        if (shader) {
            return shader;
        } else {
            TE_LOGGER_WARN("Shader not found with name: {0}", shaderName);
            return nullptr;
        }
    }

    Shader* GpuResourceManager::getActiveShader() {
        if (!m_activeShader) {
            TE_LOGGER_WARN("No active shader set");
        }
        return m_activeShader;
    }
}
