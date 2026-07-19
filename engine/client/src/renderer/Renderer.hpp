#pragma once

#include "TechEngine/client/core/ExportDLL.hpp"
#include "systems/System.hpp"

#include "FrameBuffer.hpp"
#include "Line.hpp"
#include "RenderRequest.hpp"
#include "TechEngine/core/components/Components.hpp"
#include "TechEngine/core/scene/Scene.hpp"
#include "ui/UIRenderer.hpp"

#include "graph/FrameContext.hpp"
#include "graph/RenderGraph.hpp"
#include "graph/RenderResources.hpp"
#include "graph/RenderSettings.hpp"

#include "resources/GpuResourceManager.hpp"
#include "texture/SkyBox.hpp"

#include <queue>
#include <vector>

namespace TechEngine {
    class CLIENT_DLL Renderer : public System {
        struct Renderable {
            Transform* transform;
            MeshRenderer* meshRenderer;

            bool operator<(const Renderable& other) const {
                if (meshRenderer->meshUUID.toString() < other.meshRenderer->meshUUID.toString()) return true;
                if (meshRenderer->meshUUID.toString() > other.meshRenderer->meshUUID.toString()) return false;
                //return meshRenderer->material->getGpuID() < other.meshRenderer->material->getGpuID();
                return false;
            }
        };

    private:
        SystemsRegistry& m_systemsRegistry;
        GpuResourceManager m_gpuResourcesManager;
        UIRenderer m_uiRenderer;
        SkyBox m_skyBox;

        RenderResources m_resources;
        RenderGraph m_graph;

        RenderSettings m_settings;

        std::queue<RenderRequest> m_renderQueue;

        std::vector<Renderable> m_renderables;
        size_t m_commandToDraw = 0;

        bool m_enableDebugLightCulling = false;
        const glm::uvec3 m_gridSize = glm::uvec3(16, 9, 24);

        float m_currentExposure = 1.0f;

        std::vector<Line> lines;

    public:
        inline static const int SCENE_PASS = RenderMasks::SCENE;
        inline static const int UI_PASS = RenderMasks::UI;
        inline static const int LINE_PASS = RenderMasks::LINE;
        inline static const int POST_PROCESS_PASS = RenderMasks::POST_PROCESS;

        Renderer(SystemsRegistry& systemsRegistry);

        ~Renderer() override;

        Renderer(const Renderer&) = delete;

        Renderer& operator=(const Renderer&) = delete;

        Renderer(Renderer&&) noexcept = default;

        Renderer& operator=(Renderer&&) noexcept = delete;

        void init() override;

        void onStart() override;

        void onUpdate() override;

        void shutdown() override;

        void addRequest(const RenderRequest& request);

        void renderPipeline();

        void createLine(const glm::vec3& startPosition, const glm::vec3& endPosition, const glm::vec4& color);

        uint32_t createFramebuffer(uint32_t width, uint32_t height);

        FrameBuffer& getFramebuffer(uint32_t id);

        UIRenderer& getUIRenderer();

        AOProperties& getAOProperties();

        BloomProperties& getBloomProperties();

        ChromaticAberrationProperties& getChromaticAberrationProperties();

        VignetteProperties& getVignetteProperties();

        GammaProperties& getGammaProperties();

        ColorGradingProperties& getColorGradingProperties();

        FilmGrainProperties& getFilmGrainProperties();

        FogProperties& getFogProperties();

        FroxelGridProperties& getFroxelGridProperties();

        FroxelParams& getFroxelParams();

        VolumetricSettings& getVolumetricSettings();

        bool& getDebugLightCullingEnabled();

    private:
        void registerPasses();

        FrameContext makeFrameContext(const RenderRequest& request);

        void createRenderables();

        void populateLightDataBuffers() const;

        void populateObjectDataBuffers() const;
    };
}
