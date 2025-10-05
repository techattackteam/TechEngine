#pragma once

#include "TechEngine/core/resources/IResource.hpp"

#include <glm/glm.hpp>

namespace TechEngine {
    struct MaterialProperties {
        glm::vec4 albedo = glm::vec4(1.0f); // 16 bytes
        float metallic = 0.0f; // 4 byte
        float roughness = 0.5f; // 4 byte
        float ambientOcclusion = 1.0f; // 4 byte
        uint64_t albedoMapHandle = 0; // 8 byte
        uint64_t normalMapHandle = 0; // 8 byte
        uint64_t metallicMapHandle = 0; // 8 byte
        uint64_t roughnessMapHandle = 0; // 8 byte
        uint64_t ambientOcclusionMapHandle = 0; // 8 byte
        float padding[3] = {0.0f}; // Padding to align to 16 bytes
    };

    class CORE_DLL Material : public IResource {
    private:
        std::string name;
        uint32_t m_gpuID; // ID in the GPU

        MaterialProperties properties;

        int albedoMapID = -1; // 4 byte
        int normalMapID = -1; // 4 byte
        int metallicMapID = -1; // 4 byte
        int roughnessMapID = -1; // 4 byte
        int ambientOcclusionMapID = -1; // 4 byte

    public:
        Material(std::string name, uint32_t gpuID, glm::vec4 color);

        std::string& getName();

        uint32_t getGpuID() const;

        glm::vec4& getColor();

        void setColor(glm::vec4 color);

        MaterialProperties& getProperties();

        int& getAlbedoMapID();

        int& getNormalMapID();

        int& getMetallicMapID();

        int& getRoughnessMapID();

        int& getAmbientOcclusionMapID();
    };
}
