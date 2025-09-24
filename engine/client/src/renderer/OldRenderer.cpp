#include "GLFW.hpp"
#include "OldRenderer.hpp"

#include <any>

#include "DrawCommand.hpp"
#include "ErrorCatcher.hpp"
#include "FrameBuffer.hpp"
#include "IndicesBuffer.hpp"
#include "Line.hpp"
#include "VertexArray.hpp"
#include "VertexBuffer.hpp"
#include "components/ComponentsFactory.hpp"
#include "core/Logger.hpp"
#include "resources/ResourcesManager.hpp"
#include "resources/material/Material.hpp"
#include "resources/mesh/Vertex.hpp"
#include "scene/ScenesManager.hpp"
#include "systems/SystemsRegistry.hpp"
#include "ui/PanelWidget.hpp"
#include "ui/TextWidget.hpp"
#include "ui/WidgetsRegistry.hpp"

namespace TechEngine {
    void OldRenderer::init() {
        shadersManager.init();
        vertexArrays[BufferGameObjects] = new VertexArray();
        vertexBuffers[BufferGameObjects] = new VertexBuffer();
        vertexArrays[BufferGameObjects]->init();
        vertexBuffers[BufferGameObjects]->init(10000000 * sizeof(Vertex));
        // vertexArrays[BufferGameObjects]->addNewBufferOldFormat(*vertexBuffers[BufferGameObjects]);
        vertexArrays[BufferGameObjects]->addNewBuffer(*vertexBuffers[BufferGameObjects]);
        indicesBuffers[BufferGameObjects] = new IndicesBuffer();
        indicesBuffers[BufferGameObjects]->init(10000000 * sizeof(uint32_t));

        drawCommandBuffer = ShaderStorageBuffer();
        drawCommandBuffer.init(100000 * sizeof(DrawCommand));
        objectDataBuffer = ShaderStorageBuffer();
        objectDataBuffer.init(100000 * sizeof(ObjectData));
        materialsBuffer = ShaderStorageBuffer();
        materialsBuffer.init(1000 * sizeof(MaterialProperties));

        vertexArrays[BufferLines] = new VertexArray();
        vertexBuffers[BufferLines] = new VertexBuffer();
        vertexArrays[BufferLines]->init();
        vertexBuffers[BufferLines]->init(10000000 * sizeof(Line));
        vertexArrays[BufferLines]->addNewLinesBuffer(*vertexBuffers[BufferLines]);
        glEnable(GL_DEPTH_TEST);
        uiRenderer.init();
        Mesh& cubeInfo = m_systemsRegistry.getSystem<ResourcesManager>().getMesh("Cube");
        Mesh& sphereInfo = m_systemsRegistry.getSystem<ResourcesManager>().getMesh("Sphere");
        loadMesh(cubeInfo);
        loadMesh(sphereInfo);
        prepareMDIRender();
    }

    void OldRenderer::shutdown() {
        System::shutdown();
        uiRenderer.shutdown();
    }

    OldRenderer::OldRenderer(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry), shadersManager(systemsRegistry), uiRenderer(m_systemsRegistry) {
    }

    OldRenderer::~OldRenderer() {
        for (auto& frameBuffer: frameBuffers) {
            delete frameBuffer;
        }
    }

    void OldRenderer::renderPipeline(Camera& camera) {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        geometryPass(camera);
        //renderIndirect(camera);
        //renderIndirectMultiMesh(camera);
        if (!lines.empty()) {
            linePass(camera);
        }
        if (m_systemsRegistry.hasSystem<WidgetsRegistry>()) {
            uiPass();
        }
    }

    void OldRenderer::renderCustomPipeline(Camera* camera, std::vector<int>& entities) {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        vertexBuffers[BufferGameObjects]->bind();
        vertexArrays[BufferGameObjects]->bind();
        shadersManager.changeActiveShader("geometry");
        shadersManager.getActiveShader()->setUniformMatrix4f("projection", camera->getProjectionMatrix());
        shadersManager.getActiveShader()->setUniformMatrix4f("view", camera->getViewMatrix());
        //shadersManager.getActiveShader()->setUniformVec3("cameraPosition", camera->getTransform().getPosition());
        auto& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();
        scene.runSystem<Transform, MeshRenderer>([this](Transform& transform, MeshRenderer& MeshRenderer) {
            renderMesh(transform, MeshRenderer, false);
        });
        vertexBuffers[BufferGameObjects]->unBind();
        vertexArrays[BufferGameObjects]->unBind();
    }

    void OldRenderer::renderWithLightPass(Scene& scene) {
        /*for (auto* light: scene.getLights()) {
            auto* directionLight = light->getComponent<DirectionalLightComponent>();
            shadersManager.getActiveShader()->setUniformMatrix4f("lightSpaceMatrix", directionLight->getProjectionMatrix() * directionLight->getViewMatrix());
            shadersManager.getActiveShader()->setUniformVec3("lightDirection", light->getTransform().getOrientation());
            shadersManager.getActiveShader()->setUniformVec3("lightColor", directionLight->getColor());
            renderGeometryPass(scene, false);
        }*/
    }

    void OldRenderer::renderMesh(Transform& transform, MeshRenderer& MeshRenderer, bool shadow) {
        /*for (auto& pair: gameObject->getChildren()) {
            renderGameObject(pair.second, shadow);
        }*/
        glm::mat4 model = transform.getModelMatrix();
        shadersManager.getActiveShader()->setUniformMatrix4f("model", model);
        flushMeshData(&MeshRenderer);
        if (!shadow) {
            Material* material = MeshRenderer.material;
            /*
            shadersManager.getActiveShader()->setUniformVec3("material.ambient", material.getAmbient());
            shadersManager.getActiveShader()->setUniformVec3("material.diffuse", material.getDiffuse());
            shadersManager.getActiveShader()->setUniformVec3("material.specular", material.getSpecular());
            shadersManager.getActiveShader()->setUniformFloat("material.shininess", material.getShininess());
            */
            //shadersManager.getActiveShader()->setUniformBool("material.useTexture", material.getUseTexture());
            /*if (material.getUseTexture()) {
                material.getDiffuseTexture()->bind(1);
                shadersManager.getActiveShader()->setUniformInt("diffuseTexture", 1);
            }*/
        }

        vertexBuffers[BufferGameObjects]->bind();
        vertexArrays[BufferGameObjects]->bind();
        indicesBuffers[BufferGameObjects]->bind();
        glDrawElements(GL_TRIANGLES, MeshRenderer.getIndices().size(), GL_UNSIGNED_INT, 0);
    }

    void OldRenderer::renderGeometryPass(Scene& scene, bool shadow) {
        scene.runSystem<Transform, MeshRenderer>([this, shadow](Transform& transform, MeshRenderer& MeshRenderer) {
            //if (shadow) {
            //    shadersManager.changeActiveShader("shadowMap");
            //} else {
            //    shadersManager.changeActiveShader("geometry");
            //}
            renderMesh(transform, MeshRenderer, shadow);
        });
    }

    void OldRenderer::shadowPass(Scene& scene) {
        /*shadersManager.changeActiveShader("geometry");
        if (scene.isLightingActive()) {
            shadersManager.getActiveShader()->setUniformBool("isLightingActive", true);
            for (GameObject* gameObject: scene.getLights()) {
                DirectionalLightComponent* light = gameObject->getComponent<DirectionalLightComponent>();
                glViewport(0, 0, 1024, 1024));
                shadersManager.changeActiveShader("shadowMap");
                //shadowMapBuffer.bind();
                //shadowMapBuffer.clear();
                shadersManager.getActiveShader()->setUniformMatrix4f("lightSpaceMatrix", light->getProjectionMatrix() * light->getViewMatrix());
                renderGeometryPass(scene, true);
                //shadowMapBuffer.unBind();
            }
        } else {
            shadersManager.getActiveShader()->setUniformBool("isLightingActive", false);
        }*/
    }

    void OldRenderer::geometryPass(Camera& camera) {
        vertexBuffers[BufferGameObjects]->bind();
        vertexArrays[BufferGameObjects]->bind();
        shadersManager.changeActiveShader("geometryOld");
        shadersManager.getActiveShader()->setUniformMatrix4f("projection", camera.getProjectionMatrix());
        shadersManager.getActiveShader()->setUniformMatrix4f("view", camera.getViewMatrix());
        //shadersManager.getActiveShader()->setUniformVec3("cameraPosition", camera->getTransform().getPosition());

        renderGeometryPass(m_systemsRegistry.getSystem<ScenesManager>().getActiveScene(), false);
        vertexBuffers[BufferGameObjects]->unBind();
        vertexArrays[BufferGameObjects]->unBind();
    }

    void OldRenderer::uiPass() {
        uiRenderer.beginFrame();
        PanelWidget rootWidget;
        rootWidget.m_name = "Root";
        // For this test, we'll manually set its screen rect to the full viewport
        rootWidget.m_finalScreenRect = {0.0f, 0.0f, (float)uiRenderer.m_screenWidth, (float)uiRenderer.m_screenHeight};

        std::vector<std::shared_ptr<Widget>>& widgets = m_systemsRegistry.getSystem<WidgetsRegistry>().getWidgets();
        for (auto it = widgets.rbegin(); it != widgets.rend(); it++) {
            auto& widget = *it;
            if (widget) {
                widget->draw(uiRenderer);
            }
        }
        uiRenderer.endFrame();
    }

    void OldRenderer::renderIndirectInstanced(Camera& camera) {
        Scene& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();
        scene.runSystem<Transform, MeshRenderer>([this](Transform& transform, MeshRenderer& MeshRenderer) {
            flushMeshData(&MeshRenderer);
        });
        // 1. --- CPU-Side Data Generation ---
        // This part would normally be done by a "RenderSystem" as described in the doc,
        // but for now, we'll do it directly in the OldRenderer.

        // Let's assume you've loaded a single mesh (e.g., a cube) and its vertex/index data
        // is already in your "BufferGameObjects" VBO and IBO.
        // You would need a way to get the mesh's info (index count, base vertex, etc.).
        // For this example, let's hardcode it.

        // Generate a SINGLE draw command for 100 instances of one mesh.
        std::vector<DrawCommand> commands;
        DrawCommand cmd = {};
        cmd.count = 36; // Example: a cube has 36 indices
        cmd.instanceCount = 10000;
        cmd.firstIndex = 0; // The mesh is the first (and only) one in the IBO
        cmd.baseVertex = 0; // The mesh is the first (and only) one in the VBO
        cmd.baseInstance = 0; // This is the first draw call, so our instance data starts at offset 0
        commands.push_back(cmd);

        // Generate the Object Data for 100 instances (a 10x10 grid)
        std::vector<ObjectData> objectData;
        for (int i = 0; i < 100; ++i) {
            for (int j = 0; j < 100; ++j) {
                ObjectData data;
                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(i * 2.0f, j * 2.0f, 0.0f));
                data.modelMatrix = model;
                objectData.push_back(data);
            }
        }

        // 2. --- Upload to GPU ---
        drawCommandBuffer.addData(commands.data(), commands.size() * sizeof(DrawCommand));
        objectDataBuffer.addData(objectData.data(), objectData.size() * sizeof(ObjectData));


        // 3. --- Execute Draw Call ---
        //glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shadersManager.changeActiveShader("geometry"); // Or a new "indirect" shader

        // Set uniforms that are the same for all instances
        shadersManager.getActiveShader()->setUniformMatrix4f("projection", camera.getProjectionMatrix());
        shadersManager.getActiveShader()->setUniformMatrix4f("view", camera.getViewMatrix());

        // Bind the buffers to specific "binding points" that the shader will use
        drawCommandBuffer.setBindingPoint(0);
        objectDataBuffer.setBindingPoint(1);

        // Bind the VAO containing your mesh data
        vertexArrays[BufferGameObjects]->bind();
        indicesBuffers[BufferGameObjects]->bind();
        drawCommandBuffer.bind();
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawCommandBuffer.getID());
        // The indirect draw call!
        glMultiDrawElementsIndirect(
            GL_TRIANGLES, // Type of primitive
            GL_UNSIGNED_INT, // Type of the indices
            static_cast<void*>(nullptr), // Offset into the draw command buffer
            1, // Number of commands to process from the buffer
            0 // No tightly packed struct
        );

        vertexArrays[BufferGameObjects]->unBind();
    }

    void OldRenderer::renderIndirectMultiMesh(Camera& camera) {
        // --- This part is now a "setup" or "scene generation" step ---
        // In a real application, you'd load these once, not every frame.
        // For this example, we'll call it here.
        // You'll need to get actual vertex/index data for a cube and sphere.
        Mesh& cubeInfo = m_systemsRegistry.getSystem<ResourcesManager>().getMesh("Cube");
        Mesh& sphereInfo = m_systemsRegistry.getSystem<ResourcesManager>().getMesh("Sphere");


        // Get the info for our loaded meshes
        //MeshInfo cubeInfo = loadedMeshes["cube"];
        //MeshInfo sphereInfo = loadedMeshes["sphere"];

        /*// 1. --- CPU-Side Data Generation ---
        std::vector<DrawCommand> commands;
        std::vector<ObjectData> objectData;

        // Command and Data for the CUBE
        DrawCommand cubeCmd = {};
        cubeCmd.instanceCount = 1;
        cubeCmd.count = cubeInfo.indexCount;
        cubeCmd.firstIndex = cubeInfo.firstIndex;
        cubeCmd.baseVertex = cubeInfo.baseVertex;
        cubeCmd.baseInstance = 0; // Not used for this milestone's shader
        commands.push_back(cubeCmd);

        ObjectData cubeData;
        cubeData.modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, 0.0f, 0.0f));
        objectData.push_back(cubeData);


        // Command and Data for the SPHERE
        DrawCommand sphereCmd = {};
        sphereCmd.instanceCount = 1;
        sphereCmd.count = sphereInfo.indexCount;
        sphereCmd.firstIndex = sphereInfo.firstIndex;
        sphereCmd.baseVertex = sphereInfo.baseVertex;
        sphereCmd.baseInstance = 1; // Not used for this milestone's shader
        commands.push_back(sphereCmd);

        ObjectData sphereData;
        sphereData.modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 0.0f, 0.0f));
        objectData.push_back(sphereData);*/

        // 2. --- Upload to GPU ---
        //drawCommandBuffer.addData(commands.data(), commands.size() * sizeof(DrawCommand));
        //objectDataBuffer.addData(objectData.data(), objectData.size() * sizeof(ObjectData));
        shadersManager.changeActiveShader("geometry");
        shadersManager.getActiveShader()->setUniformMatrix4f("projection", camera.getProjectionMatrix());
        shadersManager.getActiveShader()->setUniformMatrix4f("view", camera.getViewMatrix());

        // 3. --- Execute Draw Call ---
        // ... (clear color, set shaders, uniforms, bind buffers as before) ...
        drawCommandBuffer.setBindingPoint(0);
        objectDataBuffer.setBindingPoint(1);
        materialsBuffer.setBindingPoint(2);

        objectDataBuffer.bind();
        vertexArrays[BufferGameObjects]->bind();
        indicesBuffers[BufferGameObjects]->bind();
        drawCommandBuffer.bind();

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawCommandBuffer.getID());

        // The indirect draw call!
        // We now tell it to process TWO commands.
        glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            (void*)0,
            commandToDraw, // The crucial change is here! We pass 2 (or more).
            0
        );
    }

    void OldRenderer::prepareMDIRender() {
        std::vector<DrawCommand> commands;
        std::vector<ObjectData> objectData;

        // This variable will track our position in the global objectData buffer.
        std::map<Mesh*, std::vector<Transform*>> groupedInstances;
        std::map<Material*, uint32_t> materialToIDMap;
        uint32_t globalInstanceOffset = 0;

        loadMaterials();
        m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().runSystem<Transform, MeshRenderer>([&](Transform& transform, MeshRenderer& meshRenderer) {
            groupedInstances[meshRenderer.mesh].push_back(&transform);
            if (materialToIDMap.find(meshRenderer.material) == materialToIDMap.end()) {
                uint32_t newID = materialToIDMap.size();
                materialToIDMap[meshRenderer.material] = newID;
            }
        });
        // Iterate over each group (e.g., all trees, then all rocks)
        for (const auto& pair: groupedInstances) {
            Mesh* mesh = pair.first;
            const std::vector<Transform*>& transforms = pair.second;

            if (transforms.empty()) {
                continue;
            }

            // --- Create ONE DrawCommand for the entire group ---
            DrawCommand cmd = {};
            cmd.instanceCount = transforms.size(); // e.g., 1000 for the cubes
            cmd.count = mesh->m_indices.size();

            // Assuming your Mesh object now stores its location in the mega-buffer
            cmd.firstIndex = mesh->firstIndex;
            cmd.baseVertex = mesh->baseVertex;

            // CRITICAL: This command's instance data starts at the current
            // end of the objectData buffer.
            cmd.baseInstance = globalInstanceOffset;
            commands.push_back(cmd);

            // --- Add all object data for this group ---
            for (int i = 0; i < transforms.size(); i++) {
                Transform* transform = transforms[i];
                ObjectData data;
                data.modelMatrix = transform->getModelMatrix();
                data.materialIndex = i % 2 == 0 ? 0 : 1; // Alternate materials for demo purposes
                objectData.push_back(data);
            }

            // --- Update the global offset for the next group ---
            globalInstanceOffset += transforms.size();
        }

        // --- Upload the completed data to the GPU ---
        if (!commands.empty()) {
            drawCommandBuffer.addData(commands.data(), commands.size() * sizeof(DrawCommand));
            objectDataBuffer.addData(objectData.data(), objectData.size() * sizeof(ObjectData));
        }

        // Store the number of commands to draw
        this->commandToDraw = commands.size();
    }

    void OldRenderer::loadMesh(Mesh& mesh) {
        std::vector<Vertex>& vertices = mesh.m_vertices;
        std::vector<int>& indices = mesh.m_indices;
        vertexBuffers[BufferGameObjects]->addData(vertices.data(), vertices.size() * sizeof(Vertex), currentVertexOffset * sizeof(Vertex));
        indicesBuffers[BufferGameObjects]->addData(indices.data(), indices.size() * sizeof(uint32_t), currentIndexOffset * sizeof(uint32_t));

        // --- Store the info for this mesh ---
        mesh.indexCount = mesh.m_indices.size();
        mesh.firstIndex = currentIndexOffset;
        mesh.baseVertex = currentVertexOffset;
        //loadedMeshes[name] = info;

        // --- Update the global offsets for the next mesh ---
        currentVertexOffset += vertices.size();
        currentIndexOffset += indices.size();
    }

    void OldRenderer::loadMaterials() {
        const std::vector<Material*>& materialManager = m_systemsRegistry.getSystem<ResourcesManager>().getAllMaterials();
        std::vector<MaterialProperties> properties;
        for (uint32_t i = 0; i < materialManager.size(); i++) {
            properties.push_back(materialManager[i]->getProperties());
        }
        materialsBuffer.addData(properties.data(), properties.size() * sizeof(MaterialProperties));
    }

    void OldRenderer::linePass(Camera& camera) {
        vertexBuffers[BufferLines]->bind();
        vertexArrays[BufferLines]->bind();
        shadersManager.changeActiveShader("line");
        shadersManager.getActiveShader()->setUniformMatrix4f("projection", camera.getProjectionMatrix());
        shadersManager.getActiveShader()->setUniformMatrix4f("view", camera.getViewMatrix());
        flushLinesData();
        vertexBuffers[BufferLines]->bind();
        vertexArrays[BufferLines]->bind();
        glDrawArrays(GL_LINES, 0, lines.size() * 2);
        vertexBuffers[BufferLines]->unBind();
        vertexArrays[BufferLines]->unBind();;
        lines.clear();
    }

    void OldRenderer::createLine(const glm::vec3& startPosition, const glm::vec3& endPosition, const glm::vec4& color) {
        Line line = Line(startPosition, endPosition, color);
        lines.push_back(line);
    }

    void OldRenderer::flushMeshData(MeshRenderer* MeshRenderer) {
        vertexBuffers[BufferGameObjects]->addData(MeshRenderer->getVertices().data(), MeshRenderer->getVertices().size() * sizeof(Vertex), 0);
        indicesBuffers[BufferGameObjects]->addData(MeshRenderer->getIndices().data(), 0, MeshRenderer->getIndices().size() * sizeof(uint32_t));
    }

    void OldRenderer::flushLinesData() {
        std::vector<LineVertex> vertices;
        for (Line line: lines) {
            vertices.push_back(line.getStart());
            vertices.push_back(line.getEnd());
        }
        vertexBuffers[BufferLines]->addData(vertices.data(), vertices.size() * sizeof(LineVertex), 0);
    }

    FrameBuffer& OldRenderer::getFramebuffer(uint32_t id) {
        for (auto& frameBuffer: frameBuffers) {
            if (frameBuffer->getID() == id) {
                return *frameBuffer;
            }
        }
        TE_LOGGER_CRITICAL("Framebuffer with id: %d not found", id);
        throw std::runtime_error("Framebuffer not found");
    }

    uint32_t OldRenderer::createFramebuffer(uint32_t width, uint32_t height) {
        FrameBuffer* frameBuffer = new FrameBuffer();
        frameBuffer->init(frameBuffers.size() + 1, width, height);
        frameBuffers.emplace_back(frameBuffer);
        return frameBuffer->getID();
    }

    ShadersManager& OldRenderer::getShadersManager() {
        return shadersManager;
    }

    UIRenderer& OldRenderer::getUIRenderer() {
        return uiRenderer;
    }
}
