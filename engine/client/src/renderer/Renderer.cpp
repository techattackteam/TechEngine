#include "Renderer.hpp"

#include <future>

#include "DrawCommand.hpp"
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
#include "TechEngine/core/resources/material/Material.hpp"
#include "TechEngine/core/resources/mesh/Vertex.hpp"
#include "TechEngine/core/scene/Scene.hpp"
#include "scene/ScenesManager.hpp"
#include "TechEngine/core/events/scene/meshRenderer/ChangeMeshEvent.hpp"
#include "ui/PanelWidget.hpp"
#include "ui/WidgetsRegistry.hpp"
#include "window/Viewport.hpp"


namespace TechEngine {
    Renderer::Renderer(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry), m_shadersManager(systemsRegistry), m_uiRenderer(systemsRegistry) {
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
        depthPrePassFBO.attachColorTexture();
        depthPrePassFBO.attachDepthTexture();
        depthPrePassFBO.unBind();

        m_omniShadowFBO = createFramebuffer(1024, 1024);
        FrameBuffer& omniShadowFBO = getFramebuffer(m_omniShadowFBO);
        omniShadowFBO.bind();
        //omniShadowFBO.attachDepthCubeMapTexture(1024, 1024);
        omniShadowFBO.unBind();

        m_vertexArrays[BufferLines] = VertexArray();
        m_vertexBuffers[BufferLines] = VertexBuffer();
        m_vertexArrays[BufferLines].init();
        m_vertexBuffers[BufferLines].init(10000000 * sizeof(Line));
        m_vertexArrays[BufferLines].addNewLinesBuffer(m_vertexBuffers[BufferLines]);

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

        createCubeMapForIBL();
        createIrradianceMap();
        createPrefilterMap();
        createBRDFLUTTexture();
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

            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);

            if (request.renderMask & SCENE_PASS) {
                scenePass(request);
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
        static uint32_t id = 1;
        FrameBuffer* frameBuffer = new FrameBuffer();
        frameBuffer->init(id++, width, height);
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

    void Renderer::createCubeMapForIBL() {
        ResourcesManager& resourceManager = m_systemsRegistry.getSystem<ResourcesManager>();
        const std::vector<TextureResource*>& textures = resourceManager.getAllTexturesOfType(HDR);
        m_shadersManager.changeActiveShader("equirectangular");
        Shader& shader = *m_shadersManager.getActiveShader();

        for (const TextureResource* resource: textures) {
            if (m_hdrTexture.getID() != 0) {
                glBindTexture(GL_TEXTURE_2D, 0);
                m_hdrTexture.deleteTexture();
                glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            }
            m_hdrTexture.uploadFromResource(*resource);

            unsigned int captureFBO;
            unsigned int captureRBO;
            glGenFramebuffers(1, &captureFBO);
            glGenRenderbuffers(1, &captureRBO);

            glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
            glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

            if (m_envCubemap != 0) {
                glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
                glDeleteTextures(1, &m_envCubemap);
                m_envCubemap = 0;
            }
            glGenTextures(1, &m_envCubemap);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubemap);
            for (unsigned int i = 0; i < 6; ++i) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

            // Create cube geometry ONCE before the loop
            unsigned int cubeVAO = 0;
            unsigned int cubeVBO = 0;

            float vertices[] = {
                // back face
                -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
                1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
                1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
                1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
                -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
                -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
                // front face
                -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
                1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                // left face
                -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
                -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                // right face
                1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                // bottom face
                -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
                1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
                1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
                -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
                // top face
                -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
                1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
                1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
                -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f
            };

            glGenVertexArrays(1, &cubeVAO);
            glGenBuffers(1, &cubeVBO);
            glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glBindVertexArray(cubeVAO);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            // convert HDR equirectangular environment map to cubemap equivalent
            shader.bind();
            shader.setUniformInt("u_equirectangularMap", 0);
            shader.setUniformMatrix4f("u_projection", captureProjection);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_hdrTexture.getID());

            glViewport(0, 0, 512, 512);
            glCullFace(GL_FRONT);

            for (unsigned int i = 0; i < 6; ++i) {
                shader.setUniformMatrix4f("u_view", captureViews[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_envCubemap, 0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // render Cube
                glBindVertexArray(cubeVAO);
                glDrawArrays(GL_TRIANGLES, 0, 36);
                glBindVertexArray(0);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glCullFace(GL_BACK);

            // Cleanup
            glDeleteVertexArrays(1, &cubeVAO);
            glDeleteBuffers(1, &cubeVBO);
            glDeleteFramebuffers(1, &captureFBO);
            glDeleteRenderbuffers(1, &captureRBO);
        }

        glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubemap);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    void Renderer::createIrradianceMap() {
        m_shadersManager.changeActiveShader("irradiance");
        Shader& shader = *m_shadersManager.getActiveShader();

        unsigned int captureFBO;
        unsigned int captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        if (m_irradianceMap != 0) {
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            m_irradianceMap = 0;
        }
        glGenTextures(1, &m_irradianceMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_irradianceMap);
        for (unsigned int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

        // Create cube geometry ONCE before the loop
        unsigned int cubeVAO = 0;
        unsigned int cubeVBO = 0;

        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
            1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
            -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
            // front face
            -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            // left face
            -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            // right face
            1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            // bottom face
            -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
            1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
            // top face
            -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f
        };

        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        shader.bind();
        shader.setUniformMatrix4f("u_projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubemap);
        shader.setUniformInt("u_envMap", 0);

        glViewport(0, 0, 32, 32);
        glCullFace(GL_FRONT);

        for (unsigned int i = 0; i < 6; ++i) {
            shader.setUniformMatrix4f("u_view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_irradianceMap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // render Cube
            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glCullFace(GL_BACK);

        // Cleanup
        glDeleteVertexArrays(1, &cubeVAO);
        glDeleteBuffers(1, &cubeVBO);
        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);
    }

    void Renderer::createPrefilterMap() {
        m_shadersManager.changeActiveShader("prefilter");
        Shader& shader = *m_shadersManager.getActiveShader();

        unsigned int captureFBO;
        unsigned int captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 128, 128);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        if (m_prefilterMap != 0) {
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            glDeleteTextures(1, &m_prefilterMap);
        }
        glGenTextures(1, &m_prefilterMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_prefilterMap);
        for (unsigned int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
        };

        // Create cube geometry ONCE before the loops
        unsigned int cubeVAO = 0;
        unsigned int cubeVBO = 0;

        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
            1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
            -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
            // front face
            -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            // left face
            -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            // right face
            1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            // bottom face
            -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
            1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
            // top face
            -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f
        };

        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        shader.bind();
        shader.setUniformMatrix4f("u_projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubemap);
        shader.setUniformInt("u_envMap", 0);

        glCullFace(GL_FRONT);

        uint32_t maxMipLevels = 5;
        for (unsigned int mip = 0; mip < maxMipLevels; mip++) {
            // resize framebuffer according to mip-level size.
            uint32_t mipWidth = 128 * std::pow(0.5, mip);
            uint32_t mipHeight = 128 * std::pow(0.5, mip);
            glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            shader.setUniformFloat("u_roughness", roughness);

            for (unsigned int i = 0; i < 6; ++i) {
                shader.setUniformMatrix4f("u_view", captureViews[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_prefilterMap, mip);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // render Cube
                glBindVertexArray(cubeVAO);
                glDrawArrays(GL_TRIANGLES, 0, 36);
                glBindVertexArray(0);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glCullFace(GL_BACK);

        // Cleanup
        glDeleteVertexArrays(1, &cubeVAO);
        glDeleteBuffers(1, &cubeVBO);
        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);
    }

    void Renderer::createBRDFLUTTexture() {
        // 1. Ensure the correct shader program is active
        m_shadersManager.changeActiveShader("brdf");
        Shader& shader = *m_shadersManager.getActiveShader();
        shader.bind(); // Bind the shader program
        // 2. Setup Framebuffer and Texture
        unsigned int captureFBO, captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);
        if (m_brdfLUTTexture != 0) {
            glBindTexture(GL_TEXTURE_2D, 0);
            glDeleteTextures(1, &m_brdfLUTTexture);
        }
        glGenTextures(1, &m_brdfLUTTexture);

        // Configure the texture
        glBindTexture(GL_TEXTURE_2D, m_brdfLUTTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Attach texture to framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_brdfLUTTexture, 0);

        // 3. Setup viewport and clear
        glViewport(0, 0, 512, 512);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        unsigned int quadVAO = 0;
        unsigned int quadVBO = 0;

        if (quadVAO == 0) {
            float quadVertices[] = {
                // positions        // texture Coords
                -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            };
            // setup plane VAO
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
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
                    for (Texture& texture: m_textures) {
                        if (texture.getHandle() == handle) {
                            handle = texture.getHandle();
                            return;
                        }
                    }
                    m_textures.emplace_back();
                    Texture& texture = m_textures.back();
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

    void Renderer::scenePass(const RenderRequest& request) {
        glm::mat4 viewMatrix = request.viewMatrix;
        glm::mat4 projectionMatrix = request.projectionMatrix;
        glm::vec2 viewport = request.viewportSize;
        float nearPlane = request.nearPlane;
        float farPlane = request.farPlane;
        depthPrePass(viewMatrix, projectionMatrix, viewport);
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

        FrameBuffer& frameBuffer = getFramebuffer(request.targetFramebufferId);
        frameBuffer.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        geometryPass(viewMatrix, projectionMatrix, viewport, farPlane);


        // Temp render Skybox
        m_shadersManager.changeActiveShader("skybox");
        Shader* shader = m_shadersManager.getActiveShader();
        shader->setUniformMatrix4f("u_view", glm::mat4(glm::mat3(viewMatrix)));
        shader->setUniformMatrix4f("u_projection", projectionMatrix);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubemap);

        shader->setUniformInt("u_envMap", 0);


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

        frameBuffer.unBind();
    }

    void Renderer::depthPrePass(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::ivec2& viewport) {
        FrameBuffer& frameBuffer = getFramebuffer(m_depthPrePassFBO);
        frameBuffer.resize(viewport.x, viewport.y);
        frameBuffer.bind();

        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // Disable color writes

        glCullFace(GL_FRONT);

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

    void Renderer::lightCulling(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::ivec2& viewport) {
        m_shadersManager.changeActiveShader("lightCulling");
        Shader* cullShader = m_shadersManager.getActiveShader();
        cullShader->bind();

        cullShader->setUniformMatrix4f("u_view", viewMatrix);
        cullShader->setUniformMatrix4f("u_inverseProjection", glm::inverse(projectionMatrix));
        cullShader->setUniformIVec2("u_screenSize", viewport);

        glActiveTexture(GL_TEXTURE0);
        FrameBuffer& frameBuffer = getFramebuffer(m_depthPrePassFBO);
        glBindTexture(GL_TEXTURE_2D, frameBuffer.depthTexture);
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

            glViewport(0, 0, 1024, 1024);

            FrameBuffer& frameBuffer = getFramebuffer(m_omniShadowFBO);
            frameBuffer.bind();
            frameBuffer.attachDepthCubeMapTexture(1024, 1024);

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

            uint32_t textureId = frameBuffer.depthCubeMapTexture;
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

            FrameBuffer& frameBuffer = getFramebuffer(m_omniShadowFBO);
            frameBuffer.bind();
            frameBuffer.attachDepthTexture(1024, 1024);

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

            uint32_t textureId = frameBuffer.depthTexture;
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
            FrameBuffer& frameBuffer = getFramebuffer(m_omniShadowFBO);
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
                frameBuffer.attachDepthTexture(resolution, resolution);

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

                uint32_t textureId = frameBuffer.depthTexture;
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
        m_shadersManager.changeActiveShader("geometry");
        m_shadersManager.getActiveShader()->setUniformMatrix4f("u_projection", projectionMatrix);
        m_shadersManager.getActiveShader()->setUniformMatrix4f("u_view", viewMatrix);
        glm::vec3 cameraPosition = glm::inverse(viewMatrix)[3];

        m_shadersManager.getActiveShader()->setUniformVec3("u_cameraPos", cameraPosition);
        m_shadersManager.getActiveShader()->setUniformIVec2("u_screenSize", viewport);
        m_shadersManager.getActiveShader()->setUniformFloat("u_farPlane", farPlane);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_irradianceMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_prefilterMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_brdfLUTTexture);

        m_shadersManager.getActiveShader()->setUniformInt("u_irradianceMap", 0);
        m_shadersManager.getActiveShader()->setUniformInt("u_prefilterMap", 1);
        m_shadersManager.getActiveShader()->setUniformInt("u_brdfLUT", 2);

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
