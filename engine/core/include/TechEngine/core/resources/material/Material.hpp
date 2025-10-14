#pragma once

#include "TechEngine/core/resources/IResource.hpp"

#include <glm/glm.hpp>

namespace TechEngine {
    struct MaterialProperties {
        // -- Block 1: 16 bytes --
        glm::vec4 albedo = glm::vec4(1.0f); // 16 bytes

        // -- Block 2: 16 bytes --
        float metallic = 0.0f; // 4 byte
        float roughness = 0.5f; // 4 byte
        float ambientOcclusion = 1.0f; // 4 byte
        float padding1 = 0.0f; // Padding to align to 16 bytes

        // -- Block 3: 16 bytes --
        glm::vec4 emission = glm::vec4(0.0, 0, 0, 1.0f); // 16 bytes

        // -- Block 4: 16 bytes --
        uint64_t albedoMapHandle = 0; // 8 byte
        uint64_t normalMapHandle = 0; // 8 byte

        // -- Block 5: 16 bytes --
        uint64_t metallicMapHandle = 0; // 8 byte
        uint64_t roughnessMapHandle = 0; // 8 byte

        // -- Block 6: 16 bytes --
        uint64_t ambientOcclusionMapHandle = 0; // 8 byte
        uint64_t emissionMapHandle = 0; // 8 byte
    };

    class CORE_DLL Material : public IResource {
    private:
        std::string name;
        uint32_t m_gpuID; // ID in the GPU

        MaterialProperties properties;

        int albedoMapID = -1;
        int normalMapID = -1;
        int metallicMapID = -1;
        int roughnessMapID = -1;
        int ambientOcclusionMapID = -1;
        int emissionMapID = -1;

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

        int& getEmissionMapID();
    };
}
