#pragma once

#include "TechEngine/core/resources/IResource.hpp"

#include <glm/glm.hpp>

namespace TechEngine {
    struct MaterialProperties {
        glm::vec4 albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // 16 bytes
        float metallic = 0.0f; // 4 byte
        float roughness = 0.5f; // 4 byte
        float ambientOcclusion = 1.0f; // 4 byte
        float _padding; // Padding to align to 16 bytes
    };

    class CORE_DLL Material : public IResource {
    private:
        std::string name;
        uint32_t m_gpuID; // ID in the GPU

        MaterialProperties properties;

    public:
        Material(std::string name, uint32_t gpuID, glm::vec4 color);

        std::string& getName();

        uint32_t getGpuID() const;

        glm::vec4& getColor();

        void setColor(glm::vec4 color);

        MaterialProperties& getProperties();
    };
}
