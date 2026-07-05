#pragma once
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "IGPUResource.hpp"
#include "TechEngine/core/core/UUID.hpp"

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

    class Material : public IGPUResource {
    private:
        MaterialProperties m_properties;
        uint32_t m_ssboSlot;

    public:
        Material(const MaterialProperties& properties, const UUID& resourceUUID);

        void setSSBOSlot(uint32_t slot);

        const uint32_t getSSBOSlot() const;

        const MaterialProperties& getProperties();
    };
}
