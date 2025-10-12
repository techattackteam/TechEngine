#pragma once
#include <glm/fwd.hpp>

#include "Texture.hpp"
#include "renderer/FrameBuffer.hpp"

namespace TechEngine {
    class ShadersManager;
    class ResourcesManager;

    class SkyBox {
    public:
        ResourcesManager& m_resourcesManager;
        ShadersManager& m_shadersManager;

        FrameBuffer m_captureFBO;

        Texture m_hdrTexture;
        Texture m_envCubeMap;
        Texture m_irradianceMap;
        Texture m_prefilterMap;
        Texture m_brdfLUTTexture;

        unsigned int m_brdfLUTTextureOld = 0;

    public:
        SkyBox(ResourcesManager& resourcesManager, ShadersManager& shadersManager);

        ~SkyBox();

        void createSkybox(const TextureResource& hdrResource);

        void renderSkybox(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

    private:
        void createCubeMapForIBL();

        void createIrradianceMap();

        void createPrefilterMap();

        void createBRDFLUTTexture();
    };
}
