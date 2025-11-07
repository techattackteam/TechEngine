#include "SkyBox.hpp"

#include "renderer/shaders/ShadersManager.hpp"
#include "resources/ResourcesManager.hpp"

#include <vector>
#include <GL/glew.h>

#include "resources/mesh/AssimpLoader.hpp"
#include "resources/mesh/AssimpLoader.hpp"
#include "resources/mesh/AssimpLoader.hpp"
#include "resources/mesh/AssimpLoader.hpp"

namespace TechEngine {
    void renderCube() {
        static unsigned int cubeVAO = 0;
        static unsigned int cubeVBO = 0;
        if (cubeVAO == 0) {
            float vertices[] = {
                // back face
                -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
                // front face
                -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
                // left face
                -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
                // right face
                1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f,
                // bottom face
                -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f,
                // top face
                -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f
            };
            glGenVertexArrays(1, &cubeVAO);
            glGenBuffers(1, &cubeVBO);
            glBindVertexArray(cubeVAO);
            glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    void renderQuad() {
        static unsigned int quadVAO = 0;
        if (quadVAO == 0) {
            float quadVertices[] = {
                -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom-left
                1.0f, 1.0f, 0.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
            };
            unsigned int quadVBO;
            glGenVertexArrays(1, &quadVAO);
            glGenBuffers(1, &quadVBO);
            glBindVertexArray(quadVAO);
            glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        }
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

    void renderSkyBoxCube() {
        unsigned int cubeVAO = 0;
        unsigned int cubeVBO = 0;
        if (cubeVAO == 0) {
            float vertices[] = {
                // back face
                -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
                1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // top-right
                -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // top-left
                // front face
                -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
                1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // bottom-right
                1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // top-right
                1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // top-right
                -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // top-left
                -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
                // left face
                -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-right
                -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-left
                -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-right
                // right face
                1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
                1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
                1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // bottom-left
                // bottom face
                -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // top-left
                1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom-left
                1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom-left
                -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
                // top face
                -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
                1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
                1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // top-right
                1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
                -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
                -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f // bottom-left
            };
            glGenVertexArrays(1, &cubeVAO);
            glGenBuffers(1, &cubeVBO);
            // fill buffer
            glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            // link vertex attributes
            glBindVertexArray(cubeVAO);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            glBindVertexArray(cubeVAO);
            glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
            glDepthFunc(GL_LEQUAL);
            glCullFace(GL_FRONT);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glDepthFunc(GL_LESS);
            glCullFace(GL_BACK);
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }

    SkyBox::SkyBox(ResourcesManager& resourcesManager,
                   ShadersManager& shadersManager) : m_resourcesManager(resourcesManager),
                                                     m_shadersManager(shadersManager) {
    }

    SkyBox::~SkyBox() = default;

    void SkyBox::createSkybox(const TextureResource& hdrResource) {
        if (m_hdrTexture.getID() != 0) {
            //TE_LOGGER_CRITICAL("Skybox already created with id: {0}", m_hdrTexture.getID());
            return;
        }
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        m_hdrTexture.uploadFromResource(hdrResource);
        m_captureFBO.init(512, 512);
        createCubeMapForIBL();
        createIrradianceMap();
        createPrefilterMap();
        createBRDFLUTTexture();
        glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    }

    void SkyBox::renderSkybox(FrameBuffer& frameBuffer, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
        frameBuffer.bind();
        m_shadersManager.changeActiveShader("skybox");
        Shader* shader = m_shadersManager.getActiveShader();
        shader->setUniformMatrix4f("u_view", glm::mat4(glm::mat3(viewMatrix)));
        shader->setUniformMatrix4f("u_projection", projectionMatrix);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubeMap.getID());

        shader->setUniformInt("u_envMap", 0);
        GLenum attachment = GL_COLOR_ATTACHMENT3;
        glDrawBuffers(1, &attachment);
        renderSkyBoxCube();
    }

    void SkyBox::createCubeMapForIBL() {
        const uint32_t width = 512;
        const uint32_t height = 512;
        m_shadersManager.changeActiveShader("equirectangular");
        Shader& shader = *m_shadersManager.getActiveShader();

        m_captureFBO.bind();
        m_captureFBO.resize(width, height);
        m_captureFBO.attachRenderBuffer(GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT24, width, height, 0);
        glViewport(0, 0, width, height);

        m_envCubeMap.create(GL_TEXTURE_CUBE_MAP, GL_RGB16F, width, height, GL_RGB, GL_FLOAT, nullptr);
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
        };


        // convert HDR equirectangular environment map to cubemap equivalent
        shader.bind();
        shader.setUniformInt("u_equirectangularMap", 0);
        shader.setUniformMatrix4f("u_projection", captureProjection);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_hdrTexture.getID());

        glCullFace(GL_FRONT);

        for (unsigned int i = 0; i < 6; ++i) {
            shader.setUniformMatrix4f("u_view", captureViews[i]);
            m_captureFBO.attachTextureLayer(GL_COLOR_ATTACHMENT0, m_envCubeMap, i);
            m_captureFBO.clear();
            renderCube();
        }

        glCullFace(GL_BACK);

        m_envCubeMap.generateMipMap();
        m_captureFBO.unBind();
    }

    void SkyBox::createIrradianceMap() {
        m_shadersManager.changeActiveShader("irradiance");
        Shader& shader = *m_shadersManager.getActiveShader();

        const uint32_t width = 32;
        const uint32_t height = 32;

        m_captureFBO.bind();
        m_captureFBO.resize(width, height);
        m_captureFBO.attachRenderBuffer(GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT24, width, height, 0);
        glViewport(0, 0, width, height);
        m_irradianceMap.create(GL_TEXTURE_CUBE_MAP, GL_RGB16F, width, height, GL_RGB, GL_FLOAT, nullptr);

        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
        };


        shader.bind();
        shader.setUniformMatrix4f("u_projection", captureProjection);
        shader.setUniformInt("u_envMap", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubeMap.getID());

        glViewport(0, 0, width, height);
        glCullFace(GL_FRONT);

        for (unsigned int i = 0; i < 6; ++i) {
            shader.setUniformMatrix4f("u_view", captureViews[i]);
            m_captureFBO.attachTextureLayer(GL_COLOR_ATTACHMENT0, m_irradianceMap, i);
            m_captureFBO.clear();

            renderCube();
        }

        m_captureFBO.unBind();
        glCullFace(GL_BACK);
    }

    void SkyBox::createPrefilterMap() {
        m_shadersManager.changeActiveShader("prefilter");
        Shader& shader = *m_shadersManager.getActiveShader();

        constexpr uint32_t width = 128;
        constexpr uint32_t height = 128;

        m_captureFBO.bind();
        m_captureFBO.resize(width, height);
        //m_captureFBO.attachRenderBuffer(GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT24, width, height);

        m_prefilterMap.create(GL_TEXTURE_CUBE_MAP, GL_RGB16F, width, height, GL_RGB, GL_FLOAT, nullptr);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_prefilterMap.getID());
        m_prefilterMap.generateMipMap();
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
        };


        shader.bind();
        shader.setUniformMatrix4f("u_projection", captureProjection);
        shader.setUniformInt("u_envMap", 0);

        glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubeMap.getID());
        glActiveTexture(GL_TEXTURE0);

        glCullFace(GL_FRONT);

        uint32_t maxMipLevels = 5;
        for (unsigned int mip = 0; mip < maxMipLevels; mip++) {
            uint32_t mipWidth = 128 * std::pow(0.5, mip);
            uint32_t mipHeight = 128 * std::pow(0.5, mip);
            m_captureFBO.attachRenderBuffer(GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT24, mipWidth, mipHeight, 0);
            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            shader.setUniformFloat("u_roughness", roughness);

            for (unsigned int i = 0; i < 6; ++i) {
                shader.setUniformMatrix4f("u_view", captureViews[i]);
                m_captureFBO.attachTextureLayer(GL_COLOR_ATTACHMENT0, m_prefilterMap, i, mip);
                m_captureFBO.clear();

                renderCube();
            }
        }

        glCullFace(GL_BACK);
        m_captureFBO.unBind();
    }

    void SkyBox::createBRDFLUTTexture() {
        m_shadersManager.changeActiveShader("brdf");
        Shader& shader = *m_shadersManager.getActiveShader();
        shader.bind();

        constexpr uint32_t width = 512;
        constexpr uint32_t height = 512;

        m_brdfLUTTexture.create(GL_TEXTURE_2D, GL_RG16F, width, height, GL_RG, GL_FLOAT);

        m_captureFBO.bind();
        m_captureFBO.resize(width, height);
        m_captureFBO.attachExternalTexture(GL_COLOR_ATTACHMENT0, m_brdfLUTTexture, 0, 0);

        glViewport(0, 0, width, height);
        m_captureFBO.clear();

        renderQuad();
        m_captureFBO.unBind();
    }
}
