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
        omniShadowFBO.attachDepthCubeMapTexture(1024, 1024);
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

        /*TODO: When create the improved Assets pipelines move from every frame buffers update to event driven updates
         *
         */

        /*eventManager.subscribe<MaterialCreatedEvent>([this](const std::shared_ptr<Event>& event) {
            //this->uploadNewMaterial(dynamic_cast<const MaterialCreatedEvent*>(event.get())->getName());
        });

        eventManager.subscribe<MaterialDeletedEvent>([this](const std::shared_ptr<Event>& event) {
            //this->removeMaterial(dynamic_cast<const MaterialDeletedEvent*>(event.get())->getName());
        });*/
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
        populateObjectDataBuffers();
        populateLightDataBuffers();
        populateMaterialDataBuffers();
        const uint32_t size = m_renderQueue.size();

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

    void Renderer::populateLightDataBuffers() const {
        struct LightData {
            glm::vec3 position;
            float padding1;
            glm::vec3 color = glm::vec3(1.0); // 12 bytes
            float radius = 10; // 4 byte
            float intensity = 1; // 4 byte
            float padding3[3] = {0.0f}; // Padding to align to 16 bytes
        };

        Scene& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();
        m_lightsBuffer.clear();
        std::vector<LightData> lights;
        scene.runSystem<Transform, PointLight>([&](Transform& transform, PointLight& pointLight) {
            lights.push_back({transform.m_position, 0.0f, pointLight.color, pointLight.radius, pointLight.intensity, {0.0f, 0.0f, 0.0f}});
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
        if (hasLight) {
            omniShadowPass(nearPlane, farPlane, viewport);
        }

        FrameBuffer& frameBuffer = getFramebuffer(request.targetFramebufferId);
        frameBuffer.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        geometryPass(viewMatrix, projectionMatrix, viewport, farPlane);
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

    void Renderer::omniShadowPass(const float nearPlane, const float farPlane, const glm::ivec2& viewport) {
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);

        std::vector<glm::mat4> shadowTransforms;
        PointLight* light;
        glm::vec3 lightPos;
        Scene& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();
        scene.runSystem<Transform, PointLight>([&](Transform& transform, PointLight& pointLight) {
            light = &pointLight;
            lightPos = transform.m_position;
        });
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

        glViewport(0, 0, 1024, 1024);

        FrameBuffer& frameBuffer = getFramebuffer(m_omniShadowFBO);
        frameBuffer.bind();
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            TE_LOGGER_ERROR("Framebuffer not complete: {0}", status);
        }
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

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

        glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            (void*)0,
            m_commandToDraw,
            0
        );

        frameBuffer.unBind();
        glViewport(0, 0, viewport.x, viewport
                   .
                   y
        );
        glDrawBuffer(GL_BACK

        );
        glReadBuffer(GL_BACK

        );
        glColorMask(GL_TRUE

                    ,
                    GL_TRUE
                    ,
                    GL_TRUE
                    ,
                    GL_TRUE
        );
    }

    void Renderer::geometryPass(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::ivec2& viewport, const float farPlane) {
        m_shadersManager.changeActiveShader("geometry");
        m_shadersManager.getActiveShader()->setUniformMatrix4f("u_projection", projectionMatrix);
        m_shadersManager.getActiveShader()->setUniformMatrix4f("u_view", viewMatrix);
        glm::vec3 cameraPosition = glm::inverse(viewMatrix)[3];
        m_shadersManager.getActiveShader()->setUniformVec3("u_cameraPos", cameraPosition);
        m_shadersManager.getActiveShader()->setUniformIVec2("u_screenSize", viewport);

        FrameBuffer& shadowFB = getFramebuffer(m_omniShadowFBO);
        m_shadersManager.getActiveShader()->setUniformFloat("u_farPlane", farPlane);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowFB.depthCubeMapTexture);
        m_shadersManager.getActiveShader()->setUniformInt("u_shadowCubeMap", 0);

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
