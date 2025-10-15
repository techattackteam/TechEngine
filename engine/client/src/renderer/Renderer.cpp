#include "Renderer.hpp"

#include <future>

#include "DrawCommand.hpp"
#include "core/timer.hpp"
#include "TechEngine/core/components/Components.hpp"
#include "events/resourcersManager/materials/MaterialCreatedEvent.hpp"
#include "events/resourcersManager/materials/MaterialDeletedEvent.hpp"
#include "events/resourcersManager/meshManager/MeshCreatedEvent.hpp"
#include "events/resourcersManager/meshManager/MeshDeletedEvent.hpp"
#include "eventSystem/EventManager.hpp"
#include "TechEngine/core/events/scene/ComponentAddedEvent.hpp"
#include "TechEngine/core/events/scene/EntityCreatedEvent.hpp"
#include "TechEngine/core/events/scene/EntityDeletedEvent.hpp"
#include "profiling/ProfiledScope.hpp"
#include "resources/ResourcesManager.hpp"
#include "resources/mesh/AssimpLoader.hpp"
#include "resources/mesh/AssimpLoader.hpp"
#include "resources/mesh/AssimpLoader.hpp"
#include "resources/mesh/AssimpLoader.hpp"
#include "resources/mesh/AssimpLoader.hpp"
#include "resources/mesh/AssimpLoader.hpp"
#include "resources/mesh/AssimpLoader.hpp"
#include "resources/mesh/AssimpLoader.hpp"
#include "resources/mesh/AssimpLoader.hpp"
#include "TechEngine/core/resources/material/Material.hpp"
#include "TechEngine/core/resources/mesh/Vertex.hpp"
#include "TechEngine/core/scene/Scene.hpp"
#include "scene/ScenesManager.hpp"
#include "TechEngine/core/events/scene/meshRenderer/ChangeMeshEvent.hpp"
#include "ui/PanelWidget.hpp"
#include "ui/WidgetsRegistry.hpp"
#include "window/Viewport.hpp"


namespace TechEngine {
    Renderer::Renderer(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry),
                                                           m_shadersManager(systemsRegistry),
                                                           m_uiRenderer(systemsRegistry),
                                                           m_skyBox(systemsRegistry.getSystem<ResourcesManager>(), m_shadersManager) {
    }

    Renderer::~Renderer() {
    }

    void Renderer::init() {
        m_shadersManager.init();

        m_vertexArrays[BufferGameObjects] = VertexArray();
        m_vertexBuffers[BufferGameObjects] = VertexBuffer();
        m_indicesBuffers[BufferGameObjects] = IndicesBuffer();

        m_vertexArrays[BufferGameObjects].init();
        m_vertexBuffers[BufferGameObjects].init(10000 * sizeof(Vertex));
        m_indicesBuffers[BufferGameObjects].init(10000 * sizeof(uint32_t));

        m_vertexArrays[BufferGameObjects].addNewBuffer(m_vertexBuffers[BufferGameObjects]);

        m_drawCommandBuffer = ShaderStorageBuffer();
        m_drawCommandBuffer.init(10000000 * sizeof(DrawCommand));
        m_objectDataBuffer = ShaderStorageBuffer();
        m_objectDataBuffer.init(10000000 * sizeof(ObjectData));
        m_materialsBuffer = ShaderStorageBuffer();
        m_materialsBuffer.init(100 * sizeof(MaterialProperties));

        m_lightsBuffer = ShaderStorageBuffer();
        m_lightsBuffer.init(1024 * sizeof(PointLight));
        m_lightsIndexBuffer = ShaderStorageBuffer();
        m_lightsIndexBuffer.init(3500000 * sizeof(uint32_t));
        m_tileInfoBuffer = ShaderStorageBuffer();

        m_histogramBuffer = ShaderStorageBuffer();
        m_histogramBuffer.init(256 * sizeof(uint32_t));

        uint32_t maxWidth = 3840; // 4K
        uint32_t maxHeight = 2160;
        uint32_t maxTilesX = (maxWidth + TILE_SIZE - 1) / TILE_SIZE;
        uint32_t maxTilesY = (maxHeight + TILE_SIZE - 1) / TILE_SIZE;
        uint32_t totalMaxTiles = maxTilesX * maxTilesY;

        m_tileInfoBuffer.init(totalMaxTiles * sizeof(TileInfo));

        m_atomicCounterBuffer = ShaderStorageBuffer();
        m_atomicCounterBuffer.init(sizeof(uint32_t));

        m_depthPrePassFBO = createFramebuffer(800, 600);
        FrameBuffer& depthPrePassFBO = getFramebuffer(m_depthPrePassFBO);
        depthPrePassFBO.bind();
        depthPrePassFBO.attachTexture(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_RGBA16F, GL_RGBA, GL_UNSIGNED_BYTE, 0, 0);
        depthPrePassFBO.attachTexture(GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, 0, 0);
        depthPrePassFBO.unBind();

        m_gtaoFBO = createFramebuffer(800, 600);
        FrameBuffer& gtaoFBO = getFramebuffer(m_gtaoFBO);
        gtaoFBO.bind();
        gtaoFBO.attachTexture(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_R8, GL_RED, GL_UNSIGNED_BYTE, 0, 0);
        gtaoFBO.unBind();

        m_gtaoPingPongFBO = createFramebuffer(800, 600);
        FrameBuffer& gtaoDenoiseFBO = getFramebuffer(m_gtaoPingPongFBO);
        gtaoDenoiseFBO.bind();
        gtaoDenoiseFBO.attachTexture(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_R8, GL_RED, GL_UNSIGNED_BYTE, 0, 0);
        gtaoDenoiseFBO.unBind();

        m_shadowFBO = createFramebuffer(1024, 1024);
        FrameBuffer& omniShadowFBO = getFramebuffer(m_shadowFBO);
        omniShadowFBO.bind();
        omniShadowFBO.attachTexture(GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT, 0, 0);
        omniShadowFBO.unBind();

        m_hdrFBO = createFramebuffer(800, 600);
        FrameBuffer& hdrFBO = getFramebuffer(m_hdrFBO);
        hdrFBO.bind();
        hdrFBO.attachTexture(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_RGBA16F, GL_RGBA, GL_FLOAT, 0, 0);
        hdrFBO.attachTexture(GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT, 0, 0);
        hdrFBO.unBind();

        m_depthNormalTexture = Texture();
        m_depthNormalTexture.create(GL_TEXTURE_2D, GL_RGBA16F, 800, 600, GL_RGBA, GL_FLOAT, nullptr);

        m_gtaoNoiseTexture = Texture();
        std::vector<glm::vec3> gtaoNoise;
        for (unsigned int i = 0; i < 16; i++) {
            // Generate random vectors, ensuring they are in the XY plane for 2D rotation
            glm::vec3 noise(
                ((float)rand() / (RAND_MAX)) * 2.0f - 1.0f,
                ((float)rand() / (RAND_MAX)) * 2.0f - 1.0f,
                0.0f);
            gtaoNoise.push_back(glm::normalize(noise));
        }


        m_gtaoNoiseTexture.create(GL_TEXTURE_2D, GL_RGB16F, 4, 4, GL_RGB, GL_FLOAT, gtaoNoise.data());
        glBindTexture(GL_TEXTURE_2D, m_gtaoNoiseTexture.getID());
        // Have to figure out a way to set this through the Texture class
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        m_gtaoTexture = Texture();
        m_gtaoTexture.create(GL_TEXTURE_2D, GL_R8, 800, 600, GL_RED, GL_UNSIGNED_BYTE, nullptr);

        m_gtaoTempTexture = Texture();
        m_gtaoTempTexture.create(GL_TEXTURE_2D, GL_R16F, 800, 600, GL_RED, GL_FLOAT, nullptr);

        QuadVertex quadVertices[4] = {
            // positions   // texCoords
            {{-1.0f, 1.0f}, {0.0f, 1.0f}},
            {{-1.0f, -1.0f}, {0.0f, 0.0f}},
            {{1.0f, 1.0f}, {1.0f, 1.0f}},
            {{1.0f, -1.0f}, {1.0f, 0.0f}},
        };
        m_vertexArrays[BufferFullscreenQuad] = VertexArray();
        m_vertexBuffers[BufferFullscreenQuad] = VertexBuffer();
        m_vertexArrays[BufferFullscreenQuad].init();
        m_vertexBuffers[BufferFullscreenQuad].init(4 * sizeof(QuadVertex));
        m_vertexArrays[BufferFullscreenQuad].addNewBuffer(m_vertexBuffers[BufferFullscreenQuad]);
        m_vertexBuffers[BufferFullscreenQuad].addData(quadVertices, 4 * sizeof(QuadVertex), 0);


        m_vertexArrays[BufferLines] = VertexArray();
        m_vertexBuffers[BufferLines] = VertexBuffer();
        m_vertexArrays[BufferLines].init();
        m_vertexBuffers[BufferLines].init(10000000 * sizeof(Line));
        m_vertexArrays[BufferLines].addNewLinesBuffer(m_vertexBuffers[BufferLines]);

        recreateBloomTexture({800, 600});

        m_uiRenderer.init();

        EventManager& eventManager = m_systemsRegistry.getSystem<EventManager>();

        eventManager.subscribe<EntityCreatedEvent>([this](const std::shared_ptr<Event>& event) {
            // Internal Renderables pointers may have changed, so I need to reconstruct the list
            this->createRenderables();
        });
        eventManager.subscribe<EntityDeletedEvent>([this](const std::shared_ptr<Event>& event) {
            // Internal Renderables pointers may have changed, so I need to reconstruct the list
            this->createRenderables();
        });

        eventManager.subscribe<ComponentAddedEvent>([this](const std::shared_ptr<Event>& event) {
            if (dynamic_cast<const ComponentAddedEvent*>(event.get())->getComponentTypeID() == ComponentType<Material>::get()) {
                // Internal Renderables pointers may have changed, so I need to reconstruct the list
                this->createRenderables();
            } else if (dynamic_cast<const ComponentAddedEvent*>(event.get())->getComponentTypeID() == ComponentType<MeshRenderer>::get()) {
                // Internal Renderables pointers may have changed, so I need to reconstruct the list
                this->createRenderables();
            }
        });

        eventManager.subscribe<ChangeMeshEvent>([this](const std::shared_ptr<Event>& event) {
            this->createRenderables();
        });

        eventManager.subscribe<MeshCreatedEvent>([this](const std::shared_ptr<Event>& event) {
            this->uploadNewMesh(dynamic_cast<const MeshCreatedEvent*>(event.get())->m_name);
        });

        eventManager.subscribe<MeshDeletedEvent>([this](const std::shared_ptr<Event>& event) {
            this->removeMesh(dynamic_cast<const MeshDeletedEvent*>(event.get())->mesh);
        });

        /*TODO: When the improved Assets pipelines is created move from every frame buffers update to event driven updates
         *
         */

        /*eventManager.subscribe<MaterialCreatedEvent>([this](const std::shared_ptr<Event>& event) {
            //this->uploadNewMaterial(dynamic_cast<const MaterialCreatedEvent*>(event.get())->getName());
        });

        eventManager.subscribe<MaterialDeletedEvent>([this](const std::shared_ptr<Event>& event) {
            //this->removeMaterial(dynamic_cast<const MaterialDeletedEvent*>(event.get())->getName());
        });*/
        for (const TextureResource* textureResource: m_systemsRegistry.getSystem<ResourcesManager>().getAllTexturesOfType(HDR)) {
            m_skyBox.createSkybox(*textureResource);
            break; // For now only one HDR texture is supported
        }
    }

    void Renderer::onStart() {
        System::onStart();
    }

    void Renderer::onUpdate() {
        renderPipeline();
    }

    void Renderer::shutdown() {
        System::shutdown();
        m_currentIndexOffset = 0;
        m_currentVertexOffset = 0;
        m_commandToDraw = 0;
        m_vertexArrays.clear();
        m_vertexBuffers.clear();
        m_indicesBuffers.clear();
    }

    void Renderer::addRequest(const RenderRequest& request) {
        m_renderQueue.emplace(request);
    }

    void Renderer::renderPipeline() {
        const uint32_t size = m_renderQueue.size();
        if (size == 0) {
            return;
        }

        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        populateObjectDataBuffers();
        populateLightDataBuffers();
        populateMaterialDataBuffers();

        for (uint32_t i = 0; i < size; i++) {
            RenderRequest& request = m_renderQueue.front();

            FrameBuffer& framebuffer = getFramebuffer(request.targetFramebufferId);
            framebuffer.bind();
            framebuffer.resize(request.viewportSize.x, request.viewportSize.y);

            framebuffer.clear();

            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);

            if (request.renderMask & SCENE_PASS) {
                scenePass(request);
            }
            if (request.renderMask & POST_PROCESS_PASS) {
                /*FrameBuffer& hdrFrameBuffer = getFramebuffer(m_hdrFBO);
                hdrFrameBuffer.bind();
                automaticExposurePass(request.viewportSize);
                hdrFrameBuffer.unBind();*/
                recreateBloomTexture(request.viewportSize);
                bloomPass(request.viewportSize);

                FrameBuffer& frameBuffer = getFramebuffer(request.targetFramebufferId);
                frameBuffer.bind();
                //frameBuffer.resize(request.viewportSize.x, request.viewportSize.y);
                //frameBuffer.clear();
                postProcessingPass();
                frameBuffer.unBind();
            }

            if (request.renderMask & UI_PASS && m_systemsRegistry.hasSystem<WidgetsRegistry>()) {
                //uiPass();
            }
            if (request.renderMask & LINE_PASS && !lines.empty()) {
                //linePass(request.viewMatrix, request.projectionMatrix);
            }
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
        FrameBuffer* frameBuffer = new FrameBuffer();
        frameBuffer->init(width, height);
        m_frameBuffers.emplace_back(frameBuffer);
        return frameBuffer->getID();
    }

    FrameBuffer& Renderer::getFramebuffer(uint32_t id) {
        for (auto& frameBuffer: m_frameBuffers) {
            if (frameBuffer->getID() == id) {
                return *frameBuffer;
            }
        }
        TE_LOGGER_CRITICAL("Framebuffer with id: %d not found", id);
        throw std::runtime_error("Framebuffer not found");
    }

    UIRenderer& Renderer::getUIRenderer() {
        return m_uiRenderer;
    }

    void Renderer::uploadNewMesh(const std::string& name) {
        Mesh& mesh = m_systemsRegistry.getSystem<ResourcesManager>().getMesh(name);
        std::vector<Vertex>& vertices = mesh.m_vertices;

        std::vector<int>& indices = mesh.m_indices;
        m_vertexBuffers[BufferGameObjects].addData(vertices.data(), vertices.size() * sizeof(Vertex), m_currentVertexOffset * sizeof(Vertex));
        m_indicesBuffers[BufferGameObjects].addData(indices.data(), indices.size() * sizeof(uint32_t), m_currentIndexOffset * sizeof(uint32_t));

        mesh.indexCount = mesh.m_indices.size();
        mesh.firstIndex = m_currentIndexOffset;
        mesh.baseVertex = m_currentVertexOffset;

        m_currentVertexOffset += vertices.size();
        m_currentIndexOffset += indices.size();
    }

    void Renderer::removeMesh(Mesh& mesh) {
        //TODO: Instead of re-uploading all the meshes, remove the specific mesh and compact the buffers
        //std::vector<Vertex>& vertices = mesh.m_vertices;
        //std::vector<int>& indices = mesh.m_indices;

        //m_vertexBuffers[BufferGameObjects].addData(nullptr, vertices.size() * sizeof(Vertex), mesh.baseVertex * sizeof(Vertex));
        //m_indicesBuffers[BufferGameObjects].addData(nullptr, indices.size() * sizeof(uint32_t), mesh.firstIndex * sizeof(uint32_t));

        //Reconstruct the VBO and IBO
        m_currentIndexOffset = 0;
        m_currentVertexOffset = 0;
        ResourcesManager& resourcesManager = m_systemsRegistry.getSystem<ResourcesManager>();
        for (const Mesh& m: resourcesManager.getAllMeshes()) {
            if (m.getName() != mesh.getName()) {
                uploadNewMesh(mesh.getName());
            }
        }
    }

    void Renderer::uploadNewMaterial(const std::string& name) {
        Material& material = m_systemsRegistry.getSystem<ResourcesManager>().getMaterial(name);
        const MaterialProperties& properties = material.getProperties();
        m_materialsBuffer.addData(&properties, sizeof(MaterialProperties), material.getGpuID() * sizeof(MaterialProperties));
    }

    void Renderer::removeMaterial(const std::string& name) {
        const std::vector<Material*>& materials = m_systemsRegistry.getSystem<ResourcesManager>().getAllMaterials();
        for (Material* m: materials) {
            uploadNewMaterial(m->getName());
        }
    }

    void Renderer::createRenderables() {
        m_renderables.clear();
        Scene& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();

        scene.runSystem<Transform, MeshRenderer>([&](Transform& transform, MeshRenderer& meshRenderer) {
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
        Mesh* mesh = m_renderables[0].meshRenderer->mesh;
        DrawCommand cmd = {};

        cmd.firstIndex = mesh->firstIndex;
        cmd.count = mesh->m_indices.size();
        cmd.baseVertex = mesh->baseVertex;

        cmd.baseInstance = 0;
        cmd.instanceCount = 0;

        uint32_t i = 0;
        for (const auto& renderable: m_renderables) {
            if (renderable.meshRenderer->mesh != mesh) {
                commands.push_back(cmd);
                mesh = renderable.meshRenderer->mesh;
                cmd.count = mesh->m_indices.size();
                cmd.firstIndex = mesh->firstIndex;
                cmd.baseVertex = mesh->baseVertex;

                cmd.baseInstance = i;
                i++;
            }
            cmd.instanceCount++;
        }
        if (cmd.instanceCount > 0) {
            commands.push_back(cmd);
        }

        if (!commands.empty()) {
            m_drawCommandBuffer.addData(commands.data(), commands.size() * sizeof(DrawCommand));
        }

        this->m_commandToDraw = commands.size();
    }

    void Renderer::populateLightDataBuffers() const {
        struct alignas(16) LightData {
            // -- Block 1: 16 bytes --
            glm::vec3 position = glm::vec3(0.0f);
            int type = 0;

            // -- Block 2: 16 bytes --
            glm::vec3 direction = glm::vec3(0.0f);
            float radius = 0.0f;

            // -- Block 3: 16 bytes --
            glm::vec3 color;
            float intensity = 0.0f;

            // -- Block 4: 16 bytes --
            float innerCutoff = 0.0f;
            float outerCutoff = 0.0f;
            int castShadow = 0;
            float _pad2;

            // -- Block 5 & 6: 32 bytes --
            uint64_t shadowTextureHandle[4] = {0}; // 8 byte

            // -- Block 7 - 10: 64 bytes * 4 = 256 bytes --
            glm::mat4 lightSpaceMatrix[4] = {glm::mat4(1.0f)}; // 64 bytes

            // -- Block 11: 16 bytes --
            float cascadeSplits[4] = {0.0f};
        };

        if (sizeof(LightData) % 16 != 0) {
            TE_LOGGER_CRITICAL("LightData struct size is not multiple of 16 bytes! Current size: {0}", sizeof(LightData));

            throw std::runtime_error("LightData struct size is not multiple of 16 bytes");
        }

        Scene& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();
        m_lightsBuffer.clear();
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

        m_lightsBuffer.addData(lights.data(), lights.size() * sizeof(LightData), 0);
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
                    objectData[j].modelMatrix = renderable.transform->getModelMatrix();
                    objectData[j].materialIndex = renderable.meshRenderer->material->getGpuID();
                }
            });
        }
        for (auto& thread: threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        m_objectDataBuffer.addData(objectData.data(), objectData.size() * sizeof(ObjectData));
    }

    void Renderer::populateMaterialDataBuffers() {
        ResourcesManager& resourceManager = m_systemsRegistry.getSystem<ResourcesManager>();
        const std::vector<Material*>& materials = resourceManager.getAllMaterials();
        std::vector<MaterialProperties> properties;
        properties.reserve(materials.size());
        for (Material* material: materials) {
            auto updateHandle = [&](int mapID, uint64_t& handle) {
                if (mapID != -1) {
                    TextureResource& resource = resourceManager.getTexture(mapID);
                    for (Texture& texture: m_materialsTextures) {
                        if (texture.getHandle() == handle) {
                            handle = texture.getHandle();
                            return;
                        }
                    }
                    m_materialsTextures.emplace_back();
                    Texture& texture = m_materialsTextures.back();
                    texture.uploadFromResource(resource);
                    texture.makeResident();
                    handle = texture.getHandle();
                }
            };

            updateHandle(material->getAlbedoMapID(), material->getProperties().albedoMapHandle);
            updateHandle(material->getMetallicMapID(), material->getProperties().metallicMapHandle);
            updateHandle(material->getRoughnessMapID(), material->getProperties().roughnessMapHandle);
            updateHandle(material->getNormalMapID(), material->getProperties().normalMapHandle);
            updateHandle(material->getAmbientOcclusionMapID(), material->getProperties().ambientOcclusionMapHandle);
            updateHandle(material->getEmissionMapID(), material->getProperties().emissionMapHandle);
            properties.push_back(material->getProperties());
        }

        m_materialsBuffer.addData(properties.data(), properties.size() * sizeof(MaterialProperties));
    }

    void Renderer::recreateBloomTexture(const glm::ivec2& viewport) {
        if (m_bloomTexture.getWidth() == viewport.x && m_bloomTexture.getHeight() == viewport.y) {
            return;
        }

        if (m_bloomTexture.getID() != 0) {
            m_bloomTexture.deleteTexture();
        }

        uint32_t width = viewport.x;
        uint32_t height = viewport.y;

        m_bloomTexture.create(GL_TEXTURE_2D, GL_RGBA16F, width, height, GL_RGBA, GL_FLOAT, nullptr);
        m_bloomIterations = floor(log2(std::min(width, height)));
        glBindTexture(GL_TEXTURE_2D, m_bloomTexture.getID());
        glTexStorage2D(GL_TEXTURE_2D, m_bloomIterations, GL_RGBA16F, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        m_bloomTempTexture.create(GL_TEXTURE_2D, GL_RGBA16F, width, height, GL_RGBA, GL_FLOAT, nullptr);
        glBindTexture(GL_TEXTURE_2D, m_bloomTempTexture.getID());
        glTexStorage2D(GL_TEXTURE_2D, m_bloomIterations, GL_RGBA16F, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);


        TE_LOGGER_INFO("Bloom texture recreated with size: {0}x{1}", width, height);
    }

    void Renderer::scenePass(const RenderRequest& request) {
        glm::mat4 viewMatrix = request.viewMatrix;
        glm::mat4 projectionMatrix = request.projectionMatrix;
        glm::vec2 viewport = request.viewportSize;
        float nearPlane = request.nearPlane;
        float farPlane = request.farPlane;
        depthPrePass(viewMatrix, projectionMatrix, viewport);
        gtaoPass(projectionMatrix, viewport);
        lightCulling(viewMatrix, projectionMatrix, viewport);

        bool hasLight = false;
        Scene& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();
        scene.runSystem<PointLight>([&](PointLight& pointLight) {
            hasLight = true;
        });
        scene.runSystem<DirectionalLight>([&](DirectionalLight& pointLight) {
            hasLight = true;
        });
        scene.runSystem<SpotLight>([&](SpotLight& pointLight) {
            hasLight = true;
        });

        if (hasLight) {
            shadowDepthPass(request);
        }

        FrameBuffer& frameBuffer = getFramebuffer(m_hdrFBO);
        frameBuffer.bind();
        frameBuffer.resize(viewport.x, viewport.y);
        frameBuffer.clear();

        geometryPass(viewMatrix, projectionMatrix, viewport, farPlane);
        m_skyBox.renderSkybox(viewMatrix, projectionMatrix);
        frameBuffer.unBind();
    }

    void Renderer::depthPrePass(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::ivec2& viewport) {
        FrameBuffer& frameBuffer = getFramebuffer(m_depthPrePassFBO);
        frameBuffer.bind();
        frameBuffer.resize(viewport.x, viewport.y);

        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);

        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // Disable color writes

        glCullFace(GL_BACK);

        m_shadersManager.changeActiveShader("depthPrePass");
        m_shadersManager.getActiveShader()->setUniformMatrix4f("u_projection", projectionMatrix);
        m_shadersManager.getActiveShader()->setUniformMatrix4f("u_view", viewMatrix);

        m_drawCommandBuffer.setBindingPoint(0);
        m_objectDataBuffer.setBindingPoint(1);
        m_objectDataBuffer.bind();
        m_vertexArrays[BufferGameObjects].bind();
        m_indicesBuffers[BufferGameObjects].bind();
        m_drawCommandBuffer.bind();

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_drawCommandBuffer.getID());

        glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            (void*)0,
            m_commandToDraw,
            0
        );
        frameBuffer.unBind();
        glDrawBuffer(GL_BACK);
        glReadBuffer(GL_BACK);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glCullFace(GL_BACK);
    }

    void Renderer::gtaoPass(const glm::mat4& projectionMatrix, const glm::ivec2& viewport) {
        // Get the normals pass
        m_shadersManager.changeActiveShader("depthToNormal");
        Shader* normalShader = m_shadersManager.getActiveShader();
        normalShader->bind();

        if (m_depthNormalTexture.getHeight() != viewport.y || m_depthNormalTexture.getWidth() != viewport.x) {
            m_depthNormalTexture.deleteTexture();
            m_depthNormalTexture.create(GL_TEXTURE_2D, GL_RGBA16F, viewport.x, viewport.y, GL_RGBA, GL_FLOAT, nullptr);
        }

        glBindImageTexture(0, m_depthNormalTexture.getID(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, getFramebuffer(m_depthPrePassFBO).getTextureID(GL_DEPTH_ATTACHMENT));
        normalShader->setUniformInt("u_depthTexture", 0);

        normalShader->setUniformMatrix4f("u_inverseProjection", glm::inverse(projectionMatrix));
        normalShader->setUniformIVec2("u_screenSize", viewport);

        const uint32_t WORKGROUP_SIZE_X = 8;
        const uint32_t WORKGROUP_SIZE_Y = 8;
        uint32_t numGroupsX = (viewport.x + WORKGROUP_SIZE_X - 1) / WORKGROUP_SIZE_X;
        uint32_t numGroupsY = (viewport.y + WORKGROUP_SIZE_Y - 1) / WORKGROUP_SIZE_Y;

        glDispatchCompute(numGroupsX, numGroupsY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        /*if (m_gtaoTexture.getHeight() != viewport.y || m_gtaoTexture.getWidth() != viewport.x) {
            m_gtaoTexture.deleteTexture();
            m_gtaoTexture.create(GL_TEXTURE_2D, GL_R8, viewport.x, viewport.y, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        }
        if (m_gtaoTempTexture.getHeight() != viewport.y || m_gtaoTempTexture.getWidth() != viewport.x) {
            m_gtaoTempTexture.deleteTexture();
            m_gtaoTempTexture.create(GL_TEXTURE_2D, GL_R16F, viewport.x, viewport.y, GL_RED, GL_FLOAT, nullptr);
        }*/

        /*FrameBuffer& frameBuffer = getFramebuffer(m_gtaoFBO);
        frameBuffer.bind();
        frameBuffer.resize(viewport.x, viewport.y);
        frameBuffer.clear();

        FrameBuffer& denoiseFrameBuffer = getFramebuffer(m_gtaoPingPongFBO);
        denoiseFrameBuffer.bind();
        denoiseFrameBuffer.resize(viewport.x, viewport.y);
        denoiseFrameBuffer.clear();
        frameBuffer.unBind();

        m_shadersManager.changeActiveShader("gtao");
        Shader* gtaoShader = m_shadersManager.getActiveShader();
        gtaoShader->bind();

        glActiveTexture(GL_TEXTURE0);
        FrameBuffer& depthFBO = getFramebuffer(m_depthPrePassFBO);
        glBindTexture(GL_TEXTURE_2D, depthFBO.getTextureID(GL_DEPTH_ATTACHMENT));
        gtaoShader->setUniformInt("u_depthTexture", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_gtaoNoiseTexture.getID());
        gtaoShader->setUniformInt("u_noiseTexture", 1);

        glBindImageTexture(0, frameBuffer.getTextureID(GL_COLOR_ATTACHMENT0), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

        gtaoShader->setUniformMatrix4f("u_projection", projectionMatrix);
        gtaoShader->setUniformMatrix4f("u_inverseProjection", glm::inverse(projectionMatrix));
        gtaoShader->setUniformIVec2("u_screenSize", viewport);

        gtaoShader->setUniformFloat("u_radius", 1.0f); // World-space radius
        gtaoShader->setUniformFloat("u_strength", 5.0f);

        const uint32_t WORKGROUP_SIZE_X = 8;
        const uint32_t WORKGROUP_SIZE_Y = 8;
        uint32_t numGroupsX = (viewport.x + WORKGROUP_SIZE_X - 1) / WORKGROUP_SIZE_X;
        uint32_t numGroupsY = (viewport.y + WORKGROUP_SIZE_Y - 1) / WORKGROUP_SIZE_Y;

        glDispatchCompute(numGroupsX, numGroupsY, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // Denoise Pass
        m_shadersManager.changeActiveShader("gtaoDenoise");
        Shader* denoiseShader = m_shadersManager.getActiveShader();
        denoiseShader->bind();

        denoiseShader->setUniformIVec2("u_axis", {1, 0});

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, frameBuffer.getTextureID(GL_COLOR_ATTACHMENT0));
        denoiseShader->setUniformInt("u_sourceTexture", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, getFramebuffer(m_depthPrePassFBO).getTextureID(GL_DEPTH_ATTACHMENT));
        denoiseShader->setUniformInt("u_depthTexture", 1);

        glBindImageTexture(0, denoiseFrameBuffer.getTextureID(GL_COLOR_ATTACHMENT0), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

        denoiseShader->setUniformMatrix4f("u_inverseProjection", glm::inverse(projectionMatrix));
        denoiseShader->setUniformFloat("u_blurSharpness", 10.0f); // Controls how much depth differences matter

        glDispatchCompute(numGroupsX, numGroupsY, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, denoiseFrameBuffer.getTextureID(GL_COLOR_ATTACHMENT0));
        denoiseShader->setUniformInt("u_sourceTexture", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, getFramebuffer(m_depthPrePassFBO).getTextureID(GL_DEPTH_ATTACHMENT));
        denoiseShader->setUniformInt("u_depthTexture", 1);

        denoiseShader->setUniformIVec2("u_axis", {0, 1});

        glBindImageTexture(0, frameBuffer.getTextureID(GL_COLOR_ATTACHMENT0), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

        glDispatchCompute(numGroupsX, numGroupsY, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);*/
    }

    void Renderer::lightCulling(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::ivec2& viewport) {
        m_shadersManager.changeActiveShader("lightCulling");
        Shader* cullShader = m_shadersManager.getActiveShader();
        cullShader->bind();

        cullShader->setUniformMatrix4f("u_view", viewMatrix);
        cullShader->setUniformMatrix4f("u_inverseProjection", glm::inverse(projectionMatrix));
        cullShader->setUniformIVec2("u_screenSize", viewport);

        glActiveTexture(GL_TEXTURE0);
        FrameBuffer& frameBuffer = getFramebuffer(m_depthPrePassFBO);
        glBindTexture(GL_TEXTURE_2D, frameBuffer.getTextureID(GL_DEPTH_ATTACHMENT));
        cullShader->setUniformInt("u_depthMap", 0);

        m_lightsBuffer.setBindingPoint(0);
        m_lightsIndexBuffer.setBindingPoint(1);
        m_tileInfoBuffer.setBindingPoint(2);
        m_atomicCounterBuffer.setBindingPoint(3);

        uint32_t zero = 0;
        glClearNamedBufferData(m_atomicCounterBuffer.getID(), GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);

        int32_t numGroupsX = (viewport.x + TILE_SIZE - 1) / TILE_SIZE;
        int32_t numGroupsY = (viewport.y + TILE_SIZE - 1) / TILE_SIZE;
        glDispatchCompute(numGroupsX, numGroupsY, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void Renderer::shadowDepthPass(const RenderRequest& request) {
        const glm::mat4 viewMatrix = request.viewMatrix;
        const glm::mat4 projectionMatrix = request.projectionMatrix;
        float nearPlane = request.nearPlane;
        float farPlane = request.farPlane;
        glm::ivec2 viewport = request.viewportSize;
        const float fov = request.fov;

        Scene& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();
        scene.runSystem<Transform, PointLight>([&](const Transform& transform, PointLight& pointLight) {
            if (pointLight.shadowTextureHandle != 0) {
                glMakeTextureHandleNonResidentARB(pointLight.shadowTextureHandle);
                pointLight.shadowTextureHandle = 0;
                glDeleteTextures(1, &pointLight.shadowMapID);
                pointLight.shadowMapID = 0;
            }

            if (!pointLight.castShadows) {
                return;
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


            FrameBuffer& frameBuffer = getFramebuffer(m_shadowFBO);
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

            m_shadersManager.changeActiveShader("omniShadowMap");
            Shader* shadowShader = m_shadersManager.getActiveShader();
            shadowShader->bind();
            for (uint32_t i = 0; i < 6; i++) {
                shadowShader->setUniformMatrix4f("u_shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
            }
            shadowShader->setUniformFloat("u_farPlane", farPlane);
            shadowShader->setUniformVec3("u_lightPos", lightPos);

            m_drawCommandBuffer.setBindingPoint(0);
            m_objectDataBuffer.setBindingPoint(1);
            m_objectDataBuffer.bind();
            m_vertexArrays[BufferGameObjects].bind();
            m_indicesBuffers[BufferGameObjects].bind();
            m_drawCommandBuffer.bind();

            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_drawCommandBuffer.getID());

            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)0, m_commandToDraw, 0);

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
            m_shadersManager.changeActiveShader("depthShadowMap");
            const glm::vec3 lightPos = transform.m_position;
            const glm::mat4 shadowProj = glm::perspective(glm::radians(spotLight.outerCutoff * 2.0f), 1.0f, nearPlane, farPlane);

            const glm::mat3 rotationMatrix = glm::mat3(transform.getModelMatrix());
            const glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
            const glm::vec3 lightDir = glm::normalize(rotationMatrix * forward);

            const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f))); // World Up
            const glm::vec3 up = glm::normalize(glm::cross(right, forward));

            const glm::mat4 shadowView = glm::lookAt(lightPos, lightPos + lightDir, up);

            spotLight.lightSpaceMatrix = shadowProj * shadowView;
            Shader* shadowShader = m_shadersManager.getActiveShader();
            shadowShader->bind();
            shadowShader->setUniformMatrix4f("u_lightMatrix", spotLight.lightSpaceMatrix);

            FrameBuffer& frameBuffer = getFramebuffer(m_shadowFBO);
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

            m_drawCommandBuffer.setBindingPoint(0);
            m_objectDataBuffer.setBindingPoint(1);
            m_objectDataBuffer.bind();
            m_vertexArrays[BufferGameObjects].bind();
            m_indicesBuffers[BufferGameObjects].bind();
            m_drawCommandBuffer.bind();

            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_drawCommandBuffer.getID());

            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)0, m_commandToDraw, 0);

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
            FrameBuffer& frameBuffer = getFramebuffer(m_shadowFBO);
            m_shadersManager.changeActiveShader("depthShadowMap");
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
                Shader* shadowShader = m_shadersManager.getActiveShader();
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

                m_drawCommandBuffer.setBindingPoint(0);
                m_objectDataBuffer.setBindingPoint(1);
                m_objectDataBuffer.bind();
                m_vertexArrays[BufferGameObjects].bind();
                m_indicesBuffers[BufferGameObjects].bind();
                m_drawCommandBuffer.bind();

                glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_drawCommandBuffer.getID());

                glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)0, m_commandToDraw, 0);

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
        populateLightDataBuffers();
    }


    void Renderer::geometryPass(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::ivec2& viewport, const float farPlane) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        m_shadersManager.changeActiveShader("geometry");
        m_shadersManager.getActiveShader()->setUniformMatrix4f("u_projection", projectionMatrix);
        m_shadersManager.getActiveShader()->setUniformMatrix4f("u_view", viewMatrix);
        glm::vec3 cameraPosition = glm::inverse(viewMatrix)[3];

        m_shadersManager.getActiveShader()->setUniformVec3("u_cameraPos", cameraPosition);
        m_shadersManager.getActiveShader()->setUniformIVec2("u_screenSize", viewport);
        m_shadersManager.getActiveShader()->setUniformFloat("u_farPlane", farPlane);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyBox.m_irradianceMap.getID());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyBox.m_prefilterMap.getID());
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_skyBox.m_brdfLUTTexture.getID());
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, getFramebuffer(m_gtaoFBO).getTextureID(GL_COLOR_ATTACHMENT0));

        m_shadersManager.getActiveShader()->setUniformInt("u_irradianceMap", 0);
        m_shadersManager.getActiveShader()->setUniformInt("u_prefilterMap", 1);
        m_shadersManager.getActiveShader()->setUniformInt("u_brdfLUT", 2);
        m_shadersManager.getActiveShader()->setUniformInt("u_aoMap", 3);

        m_drawCommandBuffer.setBindingPoint(0);
        m_objectDataBuffer.setBindingPoint(1);
        m_materialsBuffer.setBindingPoint(2);
        m_lightsBuffer.setBindingPoint(3);
        m_lightsIndexBuffer.setBindingPoint(4);
        m_tileInfoBuffer.setBindingPoint(5);

        m_objectDataBuffer.bind();
        m_vertexArrays[BufferGameObjects].bind();
        m_indicesBuffers[BufferGameObjects].bind();
        m_drawCommandBuffer.bind();

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_drawCommandBuffer.getID());

        glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            (void*)0,
            m_commandToDraw,
            0
        );

        m_objectDataBuffer.unBind();
        m_vertexArrays[BufferGameObjects].unBind();
        m_indicesBuffers[BufferGameObjects].unBind();
        m_drawCommandBuffer.unBind();
    }

    // TODO: Improve this features in future stages of the renderer
    void Renderer::automaticExposurePass(const glm::ivec2& viewport) {
        m_shadersManager.changeActiveShader("histogram");
        FrameBuffer& hdrFBO = getFramebuffer(m_hdrFBO);
        uint32_t zero = 0;


        m_histogramBuffer.bind();
        m_histogramBuffer.setBindingPoint(0);
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindImageTexture(0, hdrFBO.getTextureID(GL_COLOR_ATTACHMENT0), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);


        uint32_t numGroupsX = (viewport.x + 15) / 16;
        uint32_t numGroupsY = (viewport.y + 15) / 16;
        glDispatchCompute(numGroupsX, numGroupsY, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        uint32_t* histogramData = m_histogramBuffer.mapBuffer(GL_READ_ONLY);
        constexpr int HISTOGRAM_BINS = 256;

        if (histogramData) {
            uint32_t pixelCount = viewport.x * viewport.y;
            const float lowPercent = 0.50f; // 40% of pixels can be darker
            const float highPercent = 0.90f; // 95% of pixels should be darker (i.e., ignore top 5% brightest)

            uint32_t minPixelCount = (uint32_t)(pixelCount * lowPercent);
            uint32_t maxPixelCount = (uint32_t)(pixelCount * highPercent);

            uint32_t currentPixels = 0;
            uint32_t minBin = 0;
            uint32_t maxBin = 255;

            for (int i = 0; i < HISTOGRAM_BINS; i++) {
                currentPixels += histogramData[i];
                if (currentPixels >= maxPixelCount) {
                    maxBin = i;
                    break;
                }
            }

            for (int i = 0; i < 256; ++i) {
                currentPixels += histogramData[i];
                if (currentPixels >= minPixelCount) {
                    minBin = i;
                    break;
                }
            }

            float minLogLum = -10.0f;
            float maxLogLum = 3.0f;
            float avgLogLuminance = 0;
            uint32_t samples = 0;

            for (uint32_t i = minBin; i <= maxBin; ++i) {
                if (histogramData[i] > 0) {
                    float binLuminance = minLogLum + (i / 255.0f) * (maxLogLum - minLogLum);
                    avgLogLuminance += binLuminance * histogramData[i];
                    samples += histogramData[i];
                }
            }

            if (samples > 0) {
                avgLogLuminance /= samples;
            }

            float avgLuminance = exp(avgLogLuminance);

            float keyValue = 0.18f;
            m_targetExposure = keyValue / avgLuminance;
            m_targetExposure = std::max(0.25f, std::min(m_targetExposure, 2.0f));
            TE_LOGGER_INFO("Average Luminance: {0}, Average Log Luminance {1}, Target Exposure {2}", avgLuminance, avgLogLuminance, m_targetExposure);
        }
        m_currentExposure = m_currentExposure + (m_targetExposure - m_currentExposure);
        TE_LOGGER_INFO("Current Exposure: {0}", m_currentExposure);
        m_histogramBuffer.unmapBuffer();
        m_histogramBuffer.unBind();
    }

    void Renderer::bloomPass(const glm::ivec2& viewport) {
        m_shadersManager.changeActiveShader("bloomPrefilter");
        m_shadersManager.getActiveShader()->setUniformFloat("u_threshold", 1.0f);

        glActiveTexture(GL_TEXTURE0);
        FrameBuffer& hdrFBO = getFramebuffer(m_hdrFBO);
        glBindTexture(GL_TEXTURE_2D, hdrFBO.getTextureID(GL_COLOR_ATTACHMENT0));
        m_shadersManager.getActiveShader()->setUniformInt("u_hdrBuffer", 0);


        glBindImageTexture(0, m_bloomTexture.getID(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

        glDispatchCompute((viewport.x + 15) / 16, (viewport.y + 15) / 16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glm::ivec2 mipSize = viewport;

        m_shadersManager.changeActiveShader("bloomDownSample");

        for (int i = 0; i < m_bloomIterations - 1; i++) {
            mipSize /= 2;
            if (mipSize.x == 0 || mipSize.y == 0) break;

            m_shadersManager.getActiveShader()->setUniformIVec2("u_sourceResolution", glm::ivec2{viewport.x / pow(2.0f, i), viewport.y / pow(2.0f, i)});

            glBindImageTexture(0, m_bloomTexture.getID(), i, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
            glBindImageTexture(1, m_bloomTexture.getID(), i + 1, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

            glDispatchCompute((mipSize.x + 15) / 16, (mipSize.y + 15) / 16, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

        m_shadersManager.changeActiveShader("bloomUpSample");
        //m_shadersManager.getActiveShader()->setUniformFloat("u_intensity", 0.5f);

        for (int i = m_bloomIterations - 2; i >= 0; --i) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_bloomTexture.getID());
            m_shadersManager.getActiveShader()->setUniformInt("u_lowerMipLevel", i + 1);
            glBindImageTexture(1, m_bloomTexture.getID(), i, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

            mipSize = glm::ivec2(viewport.x, viewport.y) / (1 << i);
            glDispatchCompute((mipSize.x + 15) / 16, (mipSize.y + 15) / 16, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_bloomTexture.getID());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, m_bloomIterations - 1);
    }

    void Renderer::postProcessingPass() {
        glDisable(GL_DEPTH_TEST);

        m_shadersManager.changeActiveShader("postProcess");
        Shader* postProcessShader = m_shadersManager.getActiveShader();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, getFramebuffer(m_hdrFBO).getTextureID(GL_COLOR_ATTACHMENT0));
        postProcessShader->setUniformInt("u_hdrBuffer", 0);


        glBindTexture(GL_TEXTURE_2D, m_depthNormalTexture.getID());
        postProcessShader->setUniformInt("u_normalTexture", 0);

        //postProcessShader->setUniformFloat("u_exposure", m_currentExposure);
        postProcessShader->setUniformFloat("u_bloomStrength", 0.0f);

        postProcessShader->setUniformFloat("u_chromaticAberrationStrength", 0.0f);

        postProcessShader->setUniformFloat("u_vignetteStrength", 0.0f);
        postProcessShader->setUniformFloat("u_vignettePower", 1.0f);


        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_bloomTexture.getID());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, m_bloomIterations - 1);
        postProcessShader->setUniformInt("u_bloomBuffer", 1);

        m_vertexArrays[BufferFullscreenQuad].bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        m_vertexArrays[BufferFullscreenQuad].unBind();

        glEnable(GL_DEPTH_TEST);
    }

    // Old UI renderer will be revised
    void Renderer::uiPass() {
        m_uiRenderer.beginFrame();
        PanelWidget rootWidget;
        rootWidget.m_name = "Root";
        rootWidget.m_finalScreenRect = {0.0f, 0.0f, (float)m_uiRenderer.m_screenWidth, (float)m_uiRenderer.m_screenHeight};

        std::vector<std::shared_ptr<Widget>>& widgets = m_systemsRegistry.getSystem<WidgetsRegistry>().getWidgets();
        for (auto it = widgets.rbegin(); it != widgets.rend(); it++) {
            auto& widget = *it;
            if (widget) {
                widget->draw(m_uiRenderer);
            }
        }
        m_uiRenderer.endFrame();
    }

    void Renderer::linePass(glm::mat4 viewMatrix, glm::mat4 projectionMatrix) {
        m_shadersManager.changeActiveShader("line");
        m_shadersManager.getActiveShader()->setUniformMatrix4f("projection", projectionMatrix);
        m_shadersManager.getActiveShader()->setUniformMatrix4f("view", viewMatrix);

        // This needs to change and only do if the data has changed
        std::vector<LineVertex> vertices;
        for (Line line: lines) {
            vertices.push_back(line.getStart());
            vertices.push_back(line.getEnd());
        }
        m_vertexBuffers[BufferLines].addData(vertices.data(), vertices.size() * sizeof(LineVertex), 0);
        m_vertexBuffers[BufferLines].bind();
        m_vertexArrays[BufferLines].bind();

        glDrawArrays(GL_LINES, 0, lines.size() * 2);

        m_vertexBuffers[BufferLines].unBind();
        m_vertexArrays[BufferLines].unBind();
        lines.clear();
    }
}
