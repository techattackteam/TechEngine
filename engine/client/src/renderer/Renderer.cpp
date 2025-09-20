#include "Renderer.hpp"

#include "DrawCommand.hpp"
#include "components/Components.hpp"
#include "events/resourcersManager/materials/MaterialCreatedEvent.hpp"
#include "events/resourcersManager/materials/MaterialDeletedEvent.hpp"
#include "events/resourcersManager/meshManager/MeshCreatedEvent.hpp"
#include "events/resourcersManager/meshManager/MeshDeletedEvent.hpp"
#include "resources/ResourcesManager.hpp"
#include "resources/material/Material.hpp"
#include "resources/mesh/Vertex.hpp"
#include "scene/Scene.hpp"
#include "scene/ScenesManager.hpp"
#include "ui/PanelWidget.hpp"
#include "ui/WidgetsRegistry.hpp"


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
        m_drawCommandBuffer.init(1000 * sizeof(DrawCommand));
        m_objectDataBuffer = ShaderStorageBuffer();
        m_objectDataBuffer.init(1000 * sizeof(ObjectData));
        m_materialsBuffer = ShaderStorageBuffer();
        m_materialsBuffer.init(100 * sizeof(MaterialProperties));

        m_uiRenderer.init();
        EventDispatcher& eventDispatcher = m_systemsRegistry.getSystem<EventDispatcher>();

        eventDispatcher.subscribe<MeshCreatedEvent>([this](const std::shared_ptr<Event>& event) {
            this->uploadNewMesh(dynamic_cast<const MeshCreatedEvent*>(event.get())->m_name);
        });

        eventDispatcher.subscribe<MeshDeletedEvent>([this](const std::shared_ptr<Event>& event) {
            this->removeMesh(dynamic_cast<const MeshDeletedEvent*>(event.get())->mesh);
        });

        eventDispatcher.subscribe<MaterialCreatedEvent>([this](const std::shared_ptr<Event>& event) {
            this->uploadNewMaterial(dynamic_cast<const MaterialCreatedEvent*>(event.get())->getName());
        });

        eventDispatcher.subscribe<MaterialDeletedEvent>([this](const std::shared_ptr<Event>& event) {
            this->removeMaterial(dynamic_cast<const MaterialDeletedEvent*>(event.get())->getName());
        });
    }

    // Might upload data to GPU or prepare some resources
    void Renderer::onStart() {
        System::onStart();
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

    void Renderer::renderPipeline(Camera& camera) {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        populateDataBuffers(m_systemsRegistry.getSystem<ScenesManager>().getActiveScene());
        geometryPass(camera);
        if (m_systemsRegistry.hasSystem<WidgetsRegistry>()) {
            uiPass();
        }
        if (!lines.empty()) {
            linePass(camera);
        }
    }

    void Renderer::renderCustomPipeline(Camera& camera, int mask) {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        populateDataBuffers(m_systemsRegistry.getSystem<ScenesManager>().getActiveScene());
        if (mask & GEOMETRY_PASS) {
            geometryPass(camera);
        }
        if (mask & UI_PASS && m_systemsRegistry.hasSystem<WidgetsRegistry>()) {
            uiPass();
        }
        if (mask & LINE_PASS && !lines.empty()) {
            linePass(camera);
        }
    }

    void Renderer::createLine(const glm::vec3& startPosition, const glm::vec3& endPosition, const glm::vec4& color) {
        Line line = Line(startPosition, endPosition, color);
        lines.push_back(line);
    }

    uint32_t Renderer::createFramebuffer(uint32_t width, uint32_t height) {
        frameBuffers.emplace_back();
        FrameBuffer& frameBuffer = frameBuffers.back();
        frameBuffer.init(frameBuffers.size() + 1, width, height);
        return frameBuffer.getID();
    }

    FrameBuffer& Renderer::getFramebuffer(uint32_t id) {
        return *std::find_if(frameBuffers.begin(), frameBuffers.end(), [id](FrameBuffer& fb) {
            return fb.getID() == id;
        });
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
        for (Material* m: m_systemsRegistry.getSystem<ResourcesManager>().getAllMaterials()) {
            uploadNewMaterial(m->getName());
        }
    }

    void Renderer::populateDataBuffers(Scene& scene) {
        std::vector<DrawCommand> commands;
        std::vector<ObjectData> objectData;

        // This variable will track our position in the global objectData buffer.
        std::map<Mesh*, std::vector<std::tuple<Transform*, uint32_t>>> groupedInstances; // Mesh to list of (Transform, MaterialID)
        uint32_t globalInstanceOffset = 0;

        scene.runSystem<Transform, MeshRenderer>([&](Transform& transform, MeshRenderer& meshRenderer) {
            groupedInstances[&meshRenderer.mesh].emplace_back(&transform, meshRenderer.material.getGpuID());
        });
        for (const auto& pair: groupedInstances) {
            Mesh* mesh = pair.first;
            const std::vector<std::tuple<Transform*, uint32_t>>& properties = pair.second;

            if (properties.empty()) {
                continue;
            }

            // --- Create ONE DrawCommand for the entire group ---
            DrawCommand cmd = {};
            cmd.instanceCount = properties.size(); // e.g., 1000 for the cubes
            cmd.count = mesh->m_indices.size();

            // Assuming your Mesh object now stores its location in the mega-buffer
            cmd.firstIndex = mesh->firstIndex;
            cmd.baseVertex = mesh->baseVertex;

            // CRITICAL: This command's instance data starts at the current
            // end of the objectData buffer.
            cmd.baseInstance = globalInstanceOffset;
            commands.push_back(cmd);

            // --- Add all object data for this group ---
            for (int i = 0; i < properties.size(); i++) {
                Transform* transform = std::get<0>(properties[i]);
                ObjectData data;
                data.modelMatrix = transform->getModelMatrix();
                data.materialIndex = std::get<1>(properties[i]);
                objectData.push_back(data);
            }

            // --- Update the global offset for the next group ---
            globalInstanceOffset += properties.size();
        }

        if (!commands.empty()) {
            m_drawCommandBuffer.addData(commands.data(), commands.size() * sizeof(DrawCommand));
            m_objectDataBuffer.addData(objectData.data(), objectData.size() * sizeof(ObjectData));
        }

        // Store the number of commands to draw
        this->m_commandToDraw = commands.size();
    }

    void Renderer::geometryPass(Camera& camera) {
        //TODO: Implement Uniform Buffer Objects
        m_shadersManager.changeActiveShader("geometry");
        m_shadersManager.getActiveShader()->setUniformMatrix4f("projection", camera.getProjectionMatrix());
        m_shadersManager.getActiveShader()->setUniformMatrix4f("view", camera.getViewMatrix());

        m_drawCommandBuffer.setBindingPoint(0);
        m_objectDataBuffer.setBindingPoint(1);
        m_materialsBuffer.setBindingPoint(2);

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

    void Renderer::linePass(Camera& camera) {
        m_vertexBuffers[BufferLines].bind();
        m_vertexArrays[BufferLines].bind();

        m_shadersManager.changeActiveShader("line");
        m_shadersManager.getActiveShader()->setUniformMatrix4f("projection", camera.getProjectionMatrix());
        m_shadersManager.getActiveShader()->setUniformMatrix4f("view", camera.getViewMatrix());

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
