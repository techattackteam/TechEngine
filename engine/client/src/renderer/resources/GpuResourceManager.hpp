#pragma once
#include "GpuResourceCache.hpp"
#include "Material.hpp"
#include "Texture.hpp"
#include "renderer/mesh/IndicesBuffer.hpp"
#include "renderer/mesh/Mesh.hpp"
#include "renderer/mesh/VertexArray.hpp"
#include "renderer/mesh/VertexBuffer.hpp"
#include "renderer/shaders/Shader.hpp"
#include "renderer/shaders/ShaderStorageBuffer.hpp"

namespace TechEngine {
    class GpuResourceManager {
    public:
        SystemsRegistry& m_systemsRegistry;

        GPUResourceCache<Material> m_materialsCache;
        ShaderStorageBuffer m_materialsBuffer;


        GPUResourceCache<Mesh> m_meshesCache;
        VertexArray m_meshesVertexArray;
        VertexBuffer m_meshesVertexBuffer;
        IndicesBuffer m_meshesIndexBuffer;
        uint32_t m_currentVertexOffset = 0; // Tracks the end of the VBO data (in vertices)
        uint32_t m_currentIndexOffset = 0; // Tracks the end of the IBO data (in indices)

        GPUResourceCache<Texture> m_texturesCache;

        GPUResourceCache<Shader> m_shadersCache;
        Shader* m_activeShader = nullptr;

    public:
        GpuResourceManager(SystemsRegistry& systemsRegistry);

        void init();
#pragma region Mesh
        void uploadNewMeshResource(const UUID& meshUUID);

        void removeMeshResource(const UUID& meshUUID);

        const Mesh* getMesh(const UUID& meshUUID) const;
#pragma endregion

#pragma region Material
        void uploadNewMaterial(const UUID& materialUUID);

        void updateMaterial(const UUID& materialUUID);

        void removeMaterial(const UUID& materialUUID);

        const Material* getMaterial(const UUID& materialUUID) const;
#pragma endregion

#pragma region Texture
        void uploadNewTexture(const UUID& textureUUID);

        void removeTexture(const UUID& textureUUID);

        const Texture* getTexture(const UUID& textureUUID) const;
#pragma endregion

#pragma region Shaders
        void uploadNewShader(const UUID& shaderUUID);

        void removeShader(const UUID& shaderUUID);

        void changeActiveShader(const std::string& shaderName);

        const Shader* getShader(const UUID& shaderUUID) const;

        const Shader* getShader(const std::string& shaderName) const;

        Shader* getActiveShader();

#pragma endregion
    };
}
