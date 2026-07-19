#include "Renderer.hpp"

#include <algorithm>
#include <thread>

#include "DrawCommand.hpp"
#include "TechEngine/core/components/Components.hpp"
#include "events/resourcersManager/materials/MaterialCreatedEvent.hpp"
#include "events/resourcersManager/materials/MaterialDeletedEvent.hpp"
#include "events/resourcersManager/mesh/MeshCreatedEvent.hpp"
#include "events/resourcersManager/mesh/MeshDeletedEvent.hpp"
#include "eventSystem/EventManager.hpp"
#include "TechEngine/core/events/scene/ComponentAddedEvent.hpp"
#include "TechEngine/core/events/scene/EntityCreatedEvent.hpp"
#include "TechEngine/core/events/scene/EntityDeletedEvent.hpp"
#include "TechEngine/core/events/scene/SceneLoadEvent.hpp"
#include "profiling/ProfiledScope.hpp"
#include "resources/ResourceSystem.hpp"
#include "scene/SceneInternal.hpp"
#include "scene/SceneManager.hpp"
#include "resources/Material.hpp"
#include "TechEngine/core/scene/Scene.hpp"
#include "TechEngine/core/events/scene/meshRenderer/ChangeMeshEvent.hpp"

#include "graph/RenderTypes.hpp"
#include "passes/GBufferPass.hpp"
#include "passes/GeometryPass.hpp"
#include "passes/LightCullingPass.hpp"
#include "passes/PostProcessPass.hpp"
#include "passes/ShadowDepthPass.hpp"

namespace TechEngine {
    Renderer::Renderer(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry),
                                                           m_gpuResourcesManager(systemsRegistry),
                                                           m_uiRenderer(systemsRegistry),
                                                           m_skyBox(systemsRegistry.getSystem<ResourceSystem>()) {
    }

    Renderer::~Renderer() {
    }

    void Renderer::init() {
        m_gpuResourcesManager.init();
        m_resources.init(m_settings.froxelGrid);
        m_uiRenderer.init();

        registerPasses();
        m_graph.setup(m_resources);

        EventManager& eventManager = m_systemsRegistry.getSystem<EventManager>();

        eventManager.subscribe<EntityCreatedEvent>([this](const std::shared_ptr<Event>& event) {
            this->createRenderables();
        });
        eventManager.subscribe<EntityDeletedEvent>([this](const std::shared_ptr<Event>& event) {
            this->createRenderables();
        });

        eventManager.subscribe<SceneLoadEvent>([this](const std::shared_ptr<Event>& event) {
            this->createRenderables();
        });

        eventManager.subscribe<ComponentAddedEvent>([this](const std::shared_ptr<Event>& event) {
            if (dynamic_cast<const ComponentAddedEvent*>(event.get())->getComponentTypeID() == ComponentType<Material>::get()) {
                this->createRenderables();
            } else if (dynamic_cast<const ComponentAddedEvent*>(event.get())->getComponentTypeID() == ComponentType<MeshRenderer>::get()) {
                this->createRenderables();
            }
        });

        eventManager.subscribe<ChangeMeshEvent>([this](const std::shared_ptr<Event>& event) {
            this->createRenderables();
        });

        eventManager.subscribe<ModelCreatedEvent>([this](const std::shared_ptr<Event>& event) {
            //this->uploadNewMesh(dynamic_cast<const MeshCreatedEvent*>(event.get())->m_name);
        });

        eventManager.subscribe<MeshDeletedEvent>([this](const std::shared_ptr<Event>& event) {
            this->createRenderables();
        });
    }

    void Renderer::onStart() {
        System::onStart();
    }

    void Renderer::onUpdate() {
        renderPipeline();
    }

    void Renderer::shutdown() {
        System::shutdown();
        m_commandToDraw = 0;
        m_resources.vertexArrays.clear();
        m_resources.vertexBuffers.clear();
        m_resources.indicesBuffers.clear();
    }

    void Renderer::addRequest(const RenderRequest& request) {
        m_renderQueue.emplace(request);
    }

    void Renderer::registerPasses() {
        m_graph.addPass<GBufferPass>();
        m_graph.addPass<LightCullingPass>();
        m_graph.addPass<ShadowDepthPass>();
        m_graph.addPass<GeometryPass>();
        m_graph.addPass<PostProcessPass>();
    }

    FrameContext Renderer::makeFrameContext(const RenderRequest& request) {
        FrameContext ctx;
        ctx.viewId = request.viewId;
        ctx.cameraPosition = request.cameraPosition;
        ctx.viewMatrix = request.viewMatrix;
        ctx.projectionMatrix = request.projectionMatrix;
        ctx.nearPlane = request.nearPlane;
        ctx.farPlane = request.farPlane;
        ctx.fov = request.fov;
        ctx.targetFramebufferId = request.targetFramebufferId;
        ctx.viewportSize = request.viewportSize;
        ctx.renderMask = request.renderMask;
        ctx.createSkyBox = request.createSkyBox;

        ctx.systems = &m_systemsRegistry;
        ctx.gpu = &m_gpuResourcesManager;
        ctx.skyBox = &m_skyBox;
        ctx.settings = &m_settings;

        ctx.commandToDraw = m_commandToDraw;

        ctx.debugLightCulling = m_enableDebugLightCulling;
        ctx.gridSize = m_gridSize;
        ctx.currentExposure = &m_currentExposure;
        return ctx;
    }

    void Renderer::renderPipeline() {
        const uint32_t size = m_renderQueue.size();
        if (size == 0) {
            return;
        }

        m_systemsRegistry.getSystem<SceneManager>().getActiveScene().getInternal()->updateGlobalTransforms(); // Not ideal place for this call
        populateObjectDataBuffers();
        populateLightDataBuffers();

        for (uint32_t i = 0; i < size; i++) {
            RenderRequest& request = m_renderQueue.front();

            FrameBuffer& framebuffer = m_resources.getFramebuffer(request.targetFramebufferId);
            framebuffer.bind();
            framebuffer.resize(request.viewportSize.x, request.viewportSize.y);

            framebuffer.clear();

            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);

            FrameContext ctx = makeFrameContext(request);
            m_graph.execute(ctx, m_resources);

            framebuffer.unBind();
            m_renderQueue.pop();
        }
        glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    }

    void Renderer::createLine(const glm::vec3& startPosition, const glm::vec3& endPosition, const glm::vec4& color) {
        Line line = Line(startPosition, endPosition, color);
        lines.push_back(line);
    }

    uint32_t Renderer::createFramebuffer(uint32_t width, uint32_t height) {
        return m_resources.createFramebuffer(width, height);
    }

    FrameBuffer& Renderer::getFramebuffer(uint32_t id) {
        return m_resources.getFramebuffer(id);
    }

    UIRenderer& Renderer::getUIRenderer() {
        return m_uiRenderer;
    }

    AOProperties& Renderer::getAOProperties() {
        return m_settings.ao;
    }

    BloomProperties& Renderer::getBloomProperties() {
        return m_settings.bloom;
    }

    ChromaticAberrationProperties& Renderer::getChromaticAberrationProperties() {
        return m_settings.chromaticAberration;
    }

    VignetteProperties& Renderer::getVignetteProperties() {
        return m_settings.vignette;
    }

    GammaProperties& Renderer::getGammaProperties() {
        return m_settings.gamma;
    }

    ColorGradingProperties& Renderer::getColorGradingProperties() {
        return m_settings.colorGrading;
    }

    FilmGrainProperties& Renderer::getFilmGrainProperties() {
        return m_settings.filmGrain;
    }

    FogProperties& Renderer::getFogProperties() {
        return m_settings.fog;
    }

    FroxelGridProperties& Renderer::getFroxelGridProperties() {
        return m_settings.froxelGrid;
    }

    FroxelParams& Renderer::getFroxelParams() {
        return m_settings.froxelParams;
    }

    VolumetricSettings& Renderer::getVolumetricSettings() {
        return m_settings.volumetric;
    }

    bool& Renderer::getDebugLightCullingEnabled() {
        return m_enableDebugLightCulling;
    }

    void Renderer::createRenderables() {
        m_renderables.clear();
        Scene& scene = m_systemsRegistry.getSystem<SceneManager>().getActiveScene();

        scene.runSystem<Transform, MeshRenderer>([&](Transform& transform, MeshRenderer& meshRenderer) {
            // Skip entities whose mesh is not present on the GPU (e.g. a mesh was just deleted and
            // this entity has not yet fallen back to the default). Keeping them would dereference a
            // null Mesh below and desync the instance indexing used by populateObjectDataBuffers().
            if (m_gpuResourcesManager.getMesh(meshRenderer.meshUUID) == nullptr) {
                return;
            }
            m_renderables.emplace_back(&transform, &meshRenderer);
        });
        if (m_renderables.empty()) {
            m_commandToDraw = 0;
            return;
        }
        std::sort(m_renderables.begin(), m_renderables.end());

        std::vector<DrawCommand> commands;
        std::vector<ObjectData> objectData;

        commands.reserve(128);
        const Mesh* mesh = m_gpuResourcesManager.getMesh(m_renderables[0].meshRenderer->meshUUID);
        DrawCommand cmd = {};

        cmd.firstIndex = mesh->getFirstIndex();
        cmd.count = mesh->getIndexCount();
        cmd.baseVertex = mesh->getBaseVertex();

        cmd.baseInstance = 0;
        cmd.instanceCount = 0;

        uint32_t i = 0;
        for (const auto& renderable: m_renderables) {
            if (renderable.meshRenderer->meshUUID != mesh->getUUID()) {
                commands.push_back(cmd);
                mesh = m_gpuResourcesManager.getMesh(renderable.meshRenderer->meshUUID);
                cmd.firstIndex = mesh->getFirstIndex();
                cmd.count = mesh->getIndexCount();
                cmd.baseVertex = mesh->getBaseVertex();

                cmd.baseInstance = i;
                cmd.instanceCount = 0;
            }
            i++;
            cmd.instanceCount++;
        }
        if (cmd.instanceCount > 0) {
            commands.push_back(cmd);
        }

        if (!commands.empty()) {
            m_resources.drawCommandBuffer.addData(commands.data(), commands.size() * sizeof(DrawCommand));
        }

        this->m_commandToDraw = commands.size();
    }

    void Renderer::populateLightDataBuffers() const {
        populateLightsBuffer(m_systemsRegistry, m_resources.lightsBuffer);
    }

    void Renderer::populateObjectDataBuffers() const {
        Profiler& profiler = m_systemsRegistry.getSystem<Profiler>();
        ProfiledScope scope(profiler, "Renderer: PopulateDataBuffers");
        size_t totalSize = m_renderables.size();
        if (totalSize == 0) {
            return;
        }
        std::vector<ObjectData> objectData = std::vector<ObjectData>(totalSize);

        uint32_t numThreads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;
        threads.reserve(numThreads);
        size_t chunkSize = (totalSize + numThreads - 1) / numThreads;
        for (size_t i = 0; i < numThreads; i++) {
            size_t start = i * chunkSize;
            size_t end = std::min(start + chunkSize, totalSize);

            if (start >= end) {
                continue;
            }
            threads.emplace_back([this, &objectData, start, end]() {
                for (size_t j = start; j < end; j++) {
                    const Renderable& renderable = m_renderables[j];
                    const Material* material = m_gpuResourcesManager.getMaterial(renderable.meshRenderer->materialUUID); // Ensure material is uploaded to GPU and get its index
                    objectData[j].modelMatrix = renderable.transform->getModelMatrix();
                    objectData[j].materialIndex = material->getSSBOSlot(); // Assuming materialUUID is the index in the shader storage buffer, otherwise a mapping is needed
                }
            });
        }
        for (auto& thread: threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        m_resources.objectDataBuffer.addData(objectData.data(), objectData.size() * sizeof(ObjectData));
    }
}
