#pragma once

#include "TechEngine/client/core/ExportDLL.hpp"
#include "systems/System.hpp"

#include "shaders/ShadersManager.hpp"
#include "shaders/ShaderStorageBuffer.hpp"
#include "mesh/VertexArray.hpp"
#include "mesh/VertexBuffer.hpp"
#include "mesh/IndicesBuffer.hpp"
#include "FrameBuffer.hpp"
#include "Line.hpp"
#include "RenderRequest.hpp"
#include "TechEngine/core/components/Components.hpp"
#include "TechEngine/core/scene/Scene.hpp"
#include "ui/UIRenderer.hpp"

#include <queue>

#include "texture/SkyBox.hpp"
#include "texture/Texture.hpp"

namespace TechEngine {
    class CLIENT_DLL Renderer : public System {
        // Temp before creating the light component
        struct Light {
            glm::vec3 position; // 12 bytes
            float padding1; // 4 byte to align to 16 bytes match std430 layout
            glm::vec3 color; // 12 bytes
            float radius; // 4 byte
            float intensity; // 4 byte
            float padding3[3]; // Padding to align to 16 bytes
        };

        struct TileInfo {
            uint32_t offset;
            uint32_t lightCount;
        };

        struct Renderable {
            Transform* transform;
            MeshRenderer* meshRenderer;

            // A comparison operator for std::sort
            bool operator<(const Renderable& other) const {
                // Sort primarily by mesh, then by material, for maximum batching.
                if (meshRenderer->mesh < other.meshRenderer->mesh) return true;
                if (meshRenderer->mesh > other.meshRenderer->mesh) return false;
                // If meshes are the same, you could optionally sort by material as a secondary key.
                return meshRenderer->material->getGpuID() < other.meshRenderer->material->getGpuID();
            }
        };

        struct QuadVertex {
            glm::vec2 position;
            glm::vec2 texCoord;
        };

        struct AOProperties {
            bool enabled = true;
            int directionCount = 12;
            int stepsPerDirection = 16;
            float radius = 1.0f;
            float thickness = 0.5f;
        };

        struct BloomProperties {
            bool enabled = false;
            float threshold = 1.5f;
            float knee = 0.5f;
            float intensity = 1.0f;
        };

        struct ChromaticAberrationProperties {
            bool enabled = true;
            float strength = 0.005f;
            float offset = 1.0f;
        };

        struct VignetteProperties {
            bool enabled = false;
            float strength = 1.0f;
            float power = 1.5f;
        };

        struct GammaProperties {
            bool enabled = true;
            glm::vec3 lift = glm::vec3(0.0f);
            float liftIntensity = 1.0f;

            float gamma = 2.2f;
            glm::vec3 gammaRGB = glm::vec3(1.0f);
            float gammaIntensity = 1.0f;

            glm::vec3 gain = glm::vec3(1.0f);
            float gainIntensity = 1.0f;
        };

        struct ColorGradingProperties {
            float exposure = 1.0f;
            float saturation = 1.0f;
            float contrast = 1.0f;
            float brightness = 1.0f;

            bool useLUT = true;
            float lutStrength = 1.0f;
        };

        struct FilmGrainProperties {
            bool filmGrainEnabled = false;
            float filmGrainIntensity = 0.05f;
            float filmGrainSize = 1.0f;
        };

        struct FogProperties {
            bool enabled = false;

            float fogDensity = 0.01f;
            float fogHeightFalloff = 0.1f;
            float fogHeight = 0.0f;

            float fogStart = 0.0f;
            float fogEnd = 1.0f;
            float skyboxFogAmount = 0.5f;
            int fogBlendMode = 0;
            glm::vec3 fogColorBase = glm::vec3(0.5f, 0.5f, 0.5f);
            glm::vec3 fogColorSky = glm::vec3(0.7f, 0.7f, 0.8f);

            bool useDirectionalColor = false;
            glm::vec3 fogColorSun = glm::vec3(1.0f, 1.0f, 0.9f);
            float sunScatteringIntensity = 1.0f;

            float mieScattering = 1.0f;
            float rayleighScattering = 1.0f;
        };

        struct FroxelGridProperties {
            // Grid dimensions
            uint32_t width = 160;
            uint32_t height = 90;
            uint32_t depth = 64;

            // Depth distribution
            float nearPlane = 0.1f; // Camera near plane
            float farPlane = 100.0f; // Camera far plane
            bool useExponentialDepth = true;

            float depthDistributionScale = 1.0f;
        };

        struct FroxelParams {
            glm::mat4 viewProjectionInverse;
            glm::mat4 viewMatrix;

            glm::uvec3 froxelDimensions;
            float froxelNearPlane;

            float froxelFarPlane;
            float depthDistributionScale;
            uint32_t useExponentialDepth;
            float rcpFroxelDimX;

            float rcpFroxelDimY;
            float rcpFroxelDimZ;
            glm::vec2 padding;

            glm::vec3 cameraPosition;
            float padding2;
        };

        struct VolumetricSettings {
            bool enabled = true;
            glm::vec3 globalDensity = glm::vec3(0.01f, 0.02f, 0.01f); // Base atmospheric density
            float heightFalloff = 0.1f; // Exponential height falloff

            glm::vec3 globalAlbedo = glm::vec3(0.9f); // Scattering color (typically ~0.9)
            float anisotropy = 0.3f; // Phase function g parameter [-1, 1]

            float globalExtinction = 0.05f; // How quickly light is absorbed
            float ambientIntensity = 0.05f; // Ambient light contribution
            glm::vec2 padding;
        };

    private:
        const std::string BufferGameObjects = "GameObjects";
        const std::string BufferLines = "Lines";
        const std::string BufferFullscreenQuad = "FullscreenQuad";

        SystemsRegistry& m_systemsRegistry;

        std::queue<RenderRequest> m_renderQueue;

        ShadersManager m_shadersManager;
        std::unordered_map<std::string, VertexArray> m_vertexArrays;
        std::unordered_map<std::string, VertexBuffer> m_vertexBuffers;
        std::unordered_map<std::string, IndicesBuffer> m_indicesBuffers;

        ShaderStorageBuffer m_drawCommandBuffer;
        ShaderStorageBuffer m_objectDataBuffer;
        ShaderStorageBuffer m_materialsBuffer;
        ShaderStorageBuffer m_lightsBuffer;

        // Buffers for light culling
        ShaderStorageBuffer m_lightsIndexBuffer;
        ShaderStorageBuffer m_tileInfoBuffer;
        ShaderStorageBuffer m_atomicCounterBuffer;

        // 256 bins for the histogram
        ShaderStorageBuffer m_histogramBuffer;

        const int32_t TILE_SIZE = 16;

        uint32_t m_gBufferFBO = 0; // depht - depth Map,  0 - albedo, 1 - normals, 2 - screenLight, 3 - hdr color
        uint32_t m_shadowFBO = 0;

        std::vector<FrameBuffer*> m_frameBuffers;

        UIRenderer m_uiRenderer;

        uint32_t m_currentVertexOffset = 0; // Tracks the end of the VBO data (in vertices)
        uint32_t m_currentIndexOffset = 0; // Tracks the end of the IBO data (in indices)
        size_t m_commandToDraw = 0;

        float m_currentExposure = 1.0f;
        float m_targetExposure = 1.0f;
        float m_adaptationSpeed = 1.5f;

        std::vector<Renderable> m_renderables;

        std::vector<Texture> m_materialsTextures;

        // Ambient Occlusion
        AOProperties m_aoProperties;
        Texture m_aoTexture;

        // Bloom
        BloomProperties m_bloomProperties;
        Texture m_bloomTexture;
        Texture m_bloomTempTexture;
        int m_bloomIterations = 0;

        // Chromatic Aberration
        ChromaticAberrationProperties m_chromaticAberrationProperties;

        // Vignette
        VignetteProperties m_vignetteProperties;

        // Gama
        GammaProperties m_gammaProperties;

        // Color Grading
        Texture m_colorGradingLUT;
        ColorGradingProperties m_colorGradingProperties;

        // Film Grain
        FilmGrainProperties m_filmGrainProperties;

        // Fog
        FogProperties m_fogProperties;
        Texture m_fogTexture;

        // Volumetric Rendering
        FroxelGridProperties m_froxelGridProperties;
        FroxelParams m_froxelParams;
        VolumetricSettings m_volumetricSettings;
        Texture m_froxelTexture;
        uint32_t m_froxelParamsUBO;
        uint32_t m_volumetricSettingsUBO;

        std::vector<Line> lines;

        SkyBox m_skyBox;

    public:
        inline static const int SCENE_PASS = 1 << 0;
        inline static const int UI_PASS = 1 << 1;
        inline static const int LINE_PASS = 1 << 2;
        inline static const int POST_PROCESS_PASS = 1 << 3;

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

    private:
        void uploadNewMesh(const std::string& name);

        void removeMesh(Mesh& mesh);

        void uploadNewMaterial(const std::string& name);

        void removeMaterial(const std::string& name);

        void createRenderables();

        void createNeutralLUT(int size = 32);

        void populateLightDataBuffers() const;

        void populateObjectDataBuffers() const;

        void populateMaterialDataBuffers();

        void recreateBloomTexture(const glm::ivec2& viewport);

        void recreateFogTexture(const glm::ivec2& viewport);

        void createFroxelTexture(const glm::ivec2& viewport);

        void scenePass(const RenderRequest& request);

        void prepareGBuffer(const glm::ivec2& viewport);

        void gBufferPass(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::ivec2& viewport);

        void aoPass(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::ivec2& viewport);

        void lightCulling(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::ivec2& viewport);

        void shadowDepthPass(const RenderRequest& request);

        void fogPass(const RenderRequest& request);

        void geometryPass(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::ivec2& viewport, float farPlane);

        void automaticExposurePass(const glm::ivec2& viewport);

        void bloomPass(const glm::ivec2& viewport);

        void godRayPass(const RenderRequest& request);

        void postProcessingPass();

        void uiPass();

        void linePass(glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
    };
}
