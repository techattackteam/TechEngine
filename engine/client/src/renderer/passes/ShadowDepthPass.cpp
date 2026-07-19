#include "ShadowDepthPass.hpp"

#include <limits>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

#include "renderer/graph/FrameContext.hpp"
#include "renderer/graph/RenderResources.hpp"
#include "renderer/graph/RenderTypes.hpp"
#include "renderer/resources/GpuResourceManager.hpp"
#include "scene/SceneManager.hpp"
#include "systems/SystemsRegistry.hpp"
#include "TechEngine/core/components/Components.hpp"
#include "TechEngine/core/scene/Scene.hpp"

namespace TechEngine {
    bool ShadowDepthPass::isEnabled(const FrameContext& frame) const {
        if (!(frame.renderMask & RenderMasks::SCENE)) {
            return false;
        }
        bool hasLight = false;
        Scene& scene = frame.systems->getSystem<SceneManager>().getActiveScene();
        scene.runSystem<PointLight>([&](PointLight& pointLight) {
            hasLight = true;
        });
        scene.runSystem<DirectionalLight>([&](DirectionalLight& pointLight) {
            hasLight = true;
        });
        scene.runSystem<SpotLight>([&](SpotLight& pointLight) {
            hasLight = true;
        });
        return hasLight;
    }

    void ShadowDepthPass::execute(const FrameContext& frame, RenderResources& resources) {
        const glm::mat4 viewMatrix = frame.viewMatrix;
        const glm::mat4 projectionMatrix = frame.projectionMatrix;
        float nearPlane = frame.nearPlane;
        float farPlane = frame.farPlane;
        glm::ivec2 viewport = frame.viewportSize;
        const float fov = frame.fov;

        Scene& scene = frame.systems->getSystem<SceneManager>().getActiveScene();
        scene.runSystem<Transform, PointLight>([&](const Transform& transform, PointLight& pointLight) {
            if (!pointLight.castShadows) {
                return;
            }

            if (pointLight.shadowTextureHandle != 0) {
                glMakeTextureHandleNonResidentARB(pointLight.shadowTextureHandle);
                pointLight.shadowTextureHandle = 0;
                glDeleteTextures(1, &pointLight.shadowMapID);
                pointLight.shadowMapID = 0;
            }


            PointLight& light = pointLight;
            glm::vec3 lightPos = transform.m_position;

            std::vector<glm::mat4> shadowTransforms;
            glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));


            FrameBuffer& frameBuffer = resources.getFramebuffer(resources.shadowFBO);
            frameBuffer.bind();
            frameBuffer.attachTexture(GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT, 0, 0);
            frameBuffer.resize(1024, 1024);

            glViewport(0, 0, 1024, 1024);
            glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glCullFace(GL_FRONT);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // Disable color writes

            frame.gpu->changeActiveShader("shadowCube");
            Shader* shadowShader = frame.gpu->getActiveShader();
            shadowShader->bind();
            for (uint32_t i = 0; i < 6; i++) {
                shadowShader->setUniformMatrix4f("u_shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
            }
            shadowShader->setUniformFloat("u_farPlane", farPlane);
            shadowShader->setUniformVec3("u_lightPos", lightPos);

            resources.drawCommandBuffer.setBindingPoint(0);
            resources.objectDataBuffer.setBindingPoint(1);
            resources.objectDataBuffer.bind();
            frame.gpu->m_meshesVertexArray.bind();
            frame.gpu->m_meshesIndexBuffer.bind();
            resources.drawCommandBuffer.bind();

            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, resources.drawCommandBuffer.getID());

            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)0, frame.commandToDraw, 0);

            uint32_t textureId = frameBuffer.getTextureID(GL_DEPTH_ATTACHMENT);
            glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
            uint64_t handle = glGetTextureHandleARB(textureId);
            glMakeTextureHandleResidentARB(handle);
            light.shadowTextureHandle = handle;
            light.shadowMapID = textureId;

            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            frameBuffer.unBind();
            glViewport(0, 0, viewport.x, viewport.y);
            glDrawBuffer(GL_BACK);
            glReadBuffer(GL_BACK);
            glCullFace(GL_BACK);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        });

        scene.runSystem<Transform, SpotLight>([&](Transform& transform, SpotLight& spotLight) {
            if (spotLight.shadowTextureHandle != 0) {
                glMakeTextureHandleNonResidentARB(spotLight.shadowTextureHandle);
                spotLight.shadowTextureHandle = 0;
                glDeleteTextures(1, &spotLight.shadowMapID);
                spotLight.shadowMapID = 0;
            }

            if (!spotLight.castShadows) {
                return;
            }
            frame.gpu->changeActiveShader("depthShadowMap");
            const glm::vec3 lightPos = transform.m_position;
            const glm::mat4 shadowProj = glm::perspective(glm::radians(spotLight.outerCutoff * 2.0f), 1.0f, nearPlane, farPlane);

            const glm::mat3 rotationMatrix = glm::mat3(transform.getModelMatrix());
            const glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
            const glm::vec3 lightDir = glm::normalize(rotationMatrix * forward);

            const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f))); // World Up
            const glm::vec3 up = glm::normalize(glm::cross(right, forward));

            const glm::mat4 shadowView = glm::lookAt(lightPos, lightPos + lightDir, up);

            spotLight.lightSpaceMatrix = shadowProj * shadowView;
            Shader* shadowShader = frame.gpu->getActiveShader();
            shadowShader->bind();
            shadowShader->setUniformMatrix4f("u_lightMatrix", spotLight.lightSpaceMatrix);

            FrameBuffer& frameBuffer = resources.getFramebuffer(resources.shadowFBO);
            frameBuffer.bind();
            frameBuffer.attachTexture(GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT, 0, 0);
            frameBuffer.resize(1024, 1024);

            glViewport(0, 0, 1024, 1024);
            glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glCullFace(GL_FRONT);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);

            resources.drawCommandBuffer.setBindingPoint(0);
            resources.objectDataBuffer.setBindingPoint(1);
            resources.objectDataBuffer.bind();
            frame.gpu->m_meshesVertexArray.bind();
            frame.gpu->m_meshesIndexBuffer.bind();
            resources.drawCommandBuffer.bind();

            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, resources.drawCommandBuffer.getID());

            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)0, frame.commandToDraw, 0);

            uint32_t textureId = frameBuffer.getTextureID(GL_DEPTH_ATTACHMENT);
            glBindTexture(GL_TEXTURE_2D, textureId);
            uint64_t handle = glGetTextureHandleARB(textureId);
            glMakeTextureHandleResidentARB(handle);
            spotLight.shadowTextureHandle = handle;
            spotLight.shadowMapID = textureId;

            glBindTexture(GL_TEXTURE_2D, 0);
            frameBuffer.unBind();
            glViewport(0, 0, viewport.x, viewport.y);
            glDrawBuffer(GL_BACK);
            glReadBuffer(GL_BACK);
            glCullFace(GL_BACK);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        });

        scene.runSystem<Transform, DirectionalLight>([&](Transform& transform, DirectionalLight& directionalLight) {
            FrameBuffer& frameBuffer = resources.getFramebuffer(resources.shadowFBO);
            frame.gpu->changeActiveShader("depthShadowMap");
            constexpr int SHADOW_CASCADE_COUNT = 4;
            std::vector<float> cascadeSplitDepths;

            cascadeSplitDepths.emplace_back(nearPlane);

            constexpr float lambda = 0.75f;
            for (int i = 1; i < SHADOW_CASCADE_COUNT; i++) {
                float ratio = (float)(i) / (float)(SHADOW_CASCADE_COUNT);
                float log = nearPlane * std::pow(farPlane / nearPlane, ratio);
                float uniform = nearPlane + (farPlane - nearPlane) * ratio;
                float split = glm::mix(log, uniform, lambda);
                cascadeSplitDepths.emplace_back(split);
            }
            cascadeSplitDepths.emplace_back(farPlane);

            for (int i = 0; i < SHADOW_CASCADE_COUNT; i++) {
                if (directionalLight.shadowTextureHandle[i] != 0) {
                    glMakeTextureHandleNonResidentARB(directionalLight.shadowTextureHandle[i]);
                    directionalLight.shadowTextureHandle[i] = 0;
                    glDeleteTextures(1, &directionalLight.shadowMapID[i]);
                    directionalLight.shadowMapID[i] = 0;
                }

                if (!directionalLight.castShadows) {
                    return;
                }
                int resolution = 2048 / (1 << i);
                float cascadeNear = cascadeSplitDepths[i];
                float cascadeFar = cascadeSplitDepths[i + 1];


                glm::mat4 cascadeProjectionMatrix = glm::perspective(
                    glm::radians(fov),
                    (float)viewport.x / (float)viewport.y,
                    cascadeNear,
                    cascadeFar
                );

                const glm::vec3 lightDir = glm::normalize(transform.m_forward);

                std::vector<glm::vec3> cameraFrustumCorners = {
                    glm::vec3(-1.0f, -1.0f, 1.0f),
                    glm::vec3(1.0f, -1.0f, 1.0f),
                    glm::vec3(1.0f, 1.0f, 1.0f),
                    glm::vec3(-1.0f, 1.0f, 1.0f),
                    glm::vec3(-1.0f, -1.0f, -1.0f),
                    glm::vec3(1.0f, -1.0f, -1.0f),
                    glm::vec3(1.0f, 1.0f, -1.0f),
                    glm::vec3(-1.0f, 1.0f, -1.0f),
                };

                glm::mat4 inverse = glm::inverse(cascadeProjectionMatrix * viewMatrix);
                glm::vec3 frustumCenter = glm::vec3(0.0f);
                glm::vec3 nearPlaneCenter = glm::vec3(0.0f);
                glm::vec3 farPlaneCenter = glm::vec3(0.0f);

                for (size_t j = 0; j < cameraFrustumCorners.size(); ++j) {
                    glm::vec3& corner = cameraFrustumCorners[j];
                    glm::vec4 invCorner = inverse * glm::vec4(corner, 1.0f);
                    corner = glm::vec3(invCorner) / invCorner.w;

                    if (j < 4) {
                        nearPlaneCenter += corner;
                    } else {
                        farPlaneCenter += corner;
                    }
                }

                nearPlaneCenter /= 4.0f;
                farPlaneCenter /= 4.0f;
                frustumCenter = (nearPlaneCenter + farPlaneCenter) / 2.0f;
                const glm::vec3 lightPos = frustumCenter - lightDir * glm::length(farPlaneCenter - nearPlaneCenter) * .15f;

                glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
                if (std::abs(glm::dot(lightDir, up)) > 0.99f) {
                    up = glm::vec3(1.0f, 0.0f, 0.0f);
                }
                glm::mat4 lightView = glm::lookAt(lightPos, frustumCenter, up);

                float minX = std::numeric_limits<float>::max();
                float maxX = std::numeric_limits<float>::lowest();
                float minY = std::numeric_limits<float>::max();
                float maxY = std::numeric_limits<float>::lowest();
                float minZ = std::numeric_limits<float>::max();
                float maxZ = std::numeric_limits<float>::lowest();

                for (const auto& corner: cameraFrustumCorners) {
                    glm::vec4 lightSpaceCorner = lightView * glm::vec4(corner, 1.0f);
                    minX = std::min(minX, lightSpaceCorner.x);
                    maxX = std::max(maxX, lightSpaceCorner.x);
                    minY = std::min(minY, lightSpaceCorner.y);
                    maxY = std::max(maxY, lightSpaceCorner.y);
                    minZ = std::min(minZ, lightSpaceCorner.z);
                    maxZ = std::max(maxZ, lightSpaceCorner.z);
                }
                if (minZ > maxZ) {
                    std::swap(minZ, maxZ);
                }

                float zRange = maxZ - minZ;
                minZ -= zRange * 3.0f;
                maxZ += zRange * 3.0f;
                float xPadding = (maxX - minX) * 0.1f;
                float yPadding = (maxY - minY) * 0.1f;
                minX -= xPadding * 0.05f;
                maxX += xPadding * 0.05f;
                minY -= yPadding * 0.05f;
                maxY += yPadding * 0.05f;

                glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

                directionalLight.lightSpaceMatrix[i] = lightProjection * lightView;
                directionalLight.cascadeSplits[i] = cascadeSplitDepths[i + 1];
                Shader* shadowShader = frame.gpu->getActiveShader();
                shadowShader->bind();
                shadowShader->setUniformMatrix4f("u_lightMatrix", directionalLight.lightSpaceMatrix[i]);

                frameBuffer.bind();
                frameBuffer.attachTexture(GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT, 0, 0);
                frameBuffer.resize(resolution, resolution);
                glViewport(0, 0, resolution, resolution);
                glClear(GL_DEPTH_BUFFER_BIT);
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);
                glCullFace(GL_FRONT);
                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);

                resources.drawCommandBuffer.setBindingPoint(0);
                resources.objectDataBuffer.setBindingPoint(1);
                resources.objectDataBuffer.bind();
                frame.gpu->m_meshesVertexArray.bind();
                frame.gpu->m_meshesIndexBuffer.bind();
                resources.drawCommandBuffer.bind();

                glBindBuffer(GL_DRAW_INDIRECT_BUFFER, resources.drawCommandBuffer.getID());

                glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)0, frame.commandToDraw, 0);

                uint32_t textureId = frameBuffer.getTextureID(GL_DEPTH_ATTACHMENT);
                glBindTexture(GL_TEXTURE_2D, textureId);
                uint64_t handle = glGetTextureHandleARB(textureId);
                glMakeTextureHandleResidentARB(handle);
                directionalLight.shadowTextureHandle[i] = handle;
                directionalLight.shadowMapID[i] = textureId;

                glBindTexture(GL_TEXTURE_2D, 0);
                frameBuffer.unBind();
            }
            glViewport(0, 0, viewport.x, viewport.y);
            glDrawBuffer(GL_BACK);
            glReadBuffer(GL_BACK);
            glCullFace(GL_BACK);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        });
        populateLightsBuffer(*frame.systems, resources.lightsBuffer);
    }
}
