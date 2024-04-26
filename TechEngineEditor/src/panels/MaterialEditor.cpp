#include <filesystem>
#include <windows.h>
#include <commdlg.h>
#include "MaterialEditor.hpp"
#include "imgui.h"
#include "glm/gtc/type_ptr.hpp"
#include "UIUtils/ImGuiUtils.hpp"
#include "components/render/CameraComponent.hpp"
#include "core/Client.hpp"
#include "core/Logger.hpp"
#include "core/Server.hpp"

namespace TechEngine {
    MaterialEditor::MaterialEditor(Client& client, Server& server) : client(client), server(server), m_material(nullptr), Panel("Material Editor") {
        m_open = false;
        frameBufferID = client.renderer.createFramebuffer(1080, 720);
    }

    void MaterialEditor::init() {
        m_sphere.init(&client.materialManager.getMaterial("DefaultMaterial"));
        m_sphere.getTransform().position = glm::vec3(0, 0, 0);
        m_camera.getTransform().position = glm::vec3(0, 0, 3);
    }

    void MaterialEditor::onUpdate() {
        //Set flags for being able to close the viewport
        if (ImGui::BeginTable("Content", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Outline", 0, 250.f);
            ImGui::TableSetupColumn("Editor", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            glm::vec4 color = m_material->getColor();
            glm::vec3 ambient = m_material->getAmbient();
            glm::vec3 diffuse = m_material->getDiffuse();
            glm::vec3 specular = m_material->getSpecular();
            bool useTexture = m_material->getUseTexture();

            ImGui::BeginChild("Settings");
            ImGui::Text("Material Settings");
            ImGui::Separator();
            ImGui::Text("Name: %s", m_material->getName().c_str());
            ImGui::Separator();
            ImGui::Text("Color");
            ImGui::SameLine();
            ImGui::ColorEdit4("##Color", glm::value_ptr(color));
            ImGui::Separator();
            ImGuiUtils::drawVec3Control("Ambient", ambient, 1, 100.0f, 0, 1);
            ImGuiUtils::drawVec3Control("diffuse", diffuse, 1, 100.0f, 0, 1);
            ImGuiUtils::drawVec3Control("specular", specular, 1, 100.0f, 0, 1);

            if (color != m_material->getColor()) {
                m_material->setColor(color);
            }

            ImGui::Separator();
            ImGui::Checkbox("Use Textures", &useTexture);
            ImGui::NewLine();
            static bool open = false;
            std::string textureName = "Diffuse texture";
            if (ImGui::Button(textureName.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0)) && !open) {
                open = true;
                OPENFILENAMEA ofn;
                CHAR szFile[260] = {0};
                CHAR currentDir[256] = {0};
                ZeroMemory(&ofn, sizeof(OPENFILENAME));
                ofn.lStructSize = sizeof(OPENFILENAME);
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile);
                if (GetCurrentDirectoryA(256, currentDir))
                    ofn.lpstrInitialDir = currentDir;
                ofn.lpstrFilter = ".jpg\0*.jpg\0.png\0*.png\0\0";
                ofn.nFilterIndex = 1;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

                if (GetOpenFileNameA(&ofn) == TRUE) {
                    open = false;
                    std::string filepath = ofn.lpstrFile;
                    std::string filename = filepath.substr(filepath.find_last_of("/\\") + 1);
                    std::string materialName = filename.substr(0, filename.find_last_of("."));
                    m_material->setDiffuseTexture(&client.textureManager.getTexture(materialName));
                }
            };
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                    std::string filename = (const char*)payload->Data;
                    std::string extension = filename.substr(filename.find_last_of('.'));
                    if (extension != ".mat")
                        return;
                    std::string textureName = filename.substr(0, filename.find_last_of("."));
                    m_material->setDiffuseTexture(&client.textureManager.getTexture(textureName));
                }
                ImGui::EndDragDropTarget();
            }
            ImGui::SameLine();

            ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
            ImGui::PopStyleVar();

            ImGui::EndChild();

            ImGui::TableSetColumnIndex(1);

            ImGui::BeginChild("Material", {ImGui::GetContentRegionAvail().x, ImGui::GetWindowSize().y});
            renderObjectWithMaterial();
            ImGui::EndChild();

            ImGui::EndTable();
        }
        if (!m_open) {
            client.materialManager.serializeMaterial(m_material->getName(), m_filepath);
            std::vector<GameObject*> objects = client.sceneManager.getScene().getGameObjects();
            for (GameObject* gameObject: server.sceneManager.getScene().getGameObjects()) {
                objects.push_back(gameObject);
            }
            for (GameObject* gameObject: objects) {
                if (gameObject->hasComponent<MeshRendererComponent>()) {
                    MeshRendererComponent* meshRendererComponent = gameObject->getComponent<MeshRendererComponent>();
                    if (meshRendererComponent->getMaterial().getName() == m_material->getName()) {
                        meshRendererComponent->paintMesh();
                    }
                }
            }
        }
    }

    void MaterialEditor::renderObjectWithMaterial() {
        if (!client.sceneManager.getScene().hasMainCamera()) {
            return;
        }
        m_camera.getComponent<CameraComponent>()->update();
        m_sphere.getComponent<MeshRendererComponent>()->changeMaterial(*m_material);
        FrameBuffer& frameBuffer = client.renderer.getFramebuffer(frameBufferID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
        ImGui::Begin(name.c_str());
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        frameBuffer.bind();
        frameBuffer.resize(wsize.x, wsize.y);
        m_camera.getComponent<CameraComponent>()->updateProjectionMatrix(wsize.x / wsize.y);
        std::vector<GameObject*> objects = {&m_sphere, &m_camera};
        client.renderer.renderCustomPipeline(m_camera.getComponent<CameraComponent>(), objects);
        uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
        ImGui::Image(reinterpret_cast<void*>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        frameBuffer.unBind();
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void MaterialEditor::open(const std::string& name, const std::string& filepath) {
        m_open = true;
        m_material = &client.materialManager.getMaterial(name);
        m_sphere.getComponent<MeshRendererComponent>()->changeMaterial(*m_material);
        m_filepath = filepath;
    }
}
