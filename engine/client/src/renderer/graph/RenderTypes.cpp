#include "RenderTypes.hpp"

#include <stdexcept>
#include <vector>

#include "core/Logger.hpp"
#include "renderer/shaders/ShaderStorageBuffer.hpp"
#include "scene/SceneManager.hpp"
#include "systems/SystemsRegistry.hpp"
#include "TechEngine/core/components/Components.hpp"
#include "TechEngine/core/scene/Scene.hpp"

namespace TechEngine {
    void populateLightsBuffer(SystemsRegistry& systemsRegistry, const ShaderStorageBuffer& lightsBuffer) {
        if (sizeof(LightData) % 16 != 0) {
            TE_LOGGER_CRITICAL("LightData struct size is not multiple of 16 bytes! Current size: {0}", sizeof(LightData));

            throw std::runtime_error("LightData struct size is not multiple of 16 bytes");
        }

        Scene& scene = systemsRegistry.getSystem<SceneManager>().getActiveScene();
        lightsBuffer.clear();
        std::vector<LightData> lights;

        scene.runSystem<Transform, PointLight>([&](Transform& transform, PointLight& pointLight) {
            LightData lightData = LightData{
                .position = transform.m_position,
                .type = 0, // Point light
                .direction = glm::vec3(0.0f),
                .radius = pointLight.radius,
                .color = pointLight.color,
                .intensity = pointLight.intensity,
                .innerCutoff = 0.0f,
                .outerCutoff = 0.0f,
                .castShadow = pointLight.castShadows ? 1 : 0,
            };
            lightData.shadowTextureHandle[0] = pointLight.shadowTextureHandle;
            lightData.lightSpaceMatrix[0] = glm::mat4(1.0f);
            lights.push_back(lightData);
        });

        scene.runSystem<Transform, DirectionalLight>([&](Transform& transform, DirectionalLight& directionalLight) {
            glm::mat3 rotationMatrix = glm::mat3(transform.getModelMatrix());
            glm::vec3 modelForward = glm::vec3(0.0f, 0.0f, -1.0f);
            LightData lightData = LightData{
                .position = glm::vec3(0.0f),
                .type = 1, // Directional light
                .direction = glm::normalize(rotationMatrix * modelForward),
                .radius = 0.0f,
                .color = directionalLight.color,
                .intensity = directionalLight.intensity,
                .innerCutoff = 0.0f,
                .outerCutoff = 0.0f,
                .castShadow = directionalLight.castShadows ? 1 : 0,
            };
            if (directionalLight.shadowTextureHandle && directionalLight.lightSpaceMatrix && directionalLight.cascadeSplits) {
                for (int i = 0; i < 4; i++) {
                    lightData.shadowTextureHandle[i] = directionalLight.shadowTextureHandle[i];
                    lightData.lightSpaceMatrix[i] = directionalLight.lightSpaceMatrix[i];
                    lightData.cascadeSplits[i] = directionalLight.cascadeSplits[i];
                }
            }
            lights.push_back(lightData);
        });

        scene.runSystem<Transform, SpotLight>([&](Transform& transform, SpotLight& spotLight) {
            glm::mat3 rotationMatrix = glm::mat3(transform.getModelMatrix());
            glm::vec3 modelForward = glm::vec3(0.0f, 0.0f, -1.0f);
            LightData lightData = LightData{
                .position = transform.m_position,
                .type = 2, // Spot light
                .direction = glm::normalize(rotationMatrix * modelForward),
                .radius = 0.0f,
                .color = spotLight.color,
                .intensity = spotLight.intensity,
                .innerCutoff = glm::cos(glm::radians(spotLight.innerCutoff)),
                .outerCutoff = glm::cos(glm::radians(spotLight.outerCutoff)),
                .castShadow = spotLight.castShadows ? 1 : 0,

            };
            lightData.shadowTextureHandle[0] = spotLight.shadowTextureHandle;
            lightData.lightSpaceMatrix[0] = spotLight.lightSpaceMatrix;
            lights.push_back(lightData);
        });

        lightsBuffer.addData(lights.data(), lights.size() * sizeof(LightData), 0);
    }
}
