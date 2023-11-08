#include "MaterialEditor.hpp"
#include "imgui.h"
#include "material/MaterialManager.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "UIUtils/ImGuiUtils.hpp"
#include "components/CameraComponent.hpp"
#include "scene/SceneHelper.hpp"
#include "core/Logger.hpp"

namespace TechEngine {
    MaterialEditor::MaterialEditor(Renderer &renderer) : m_renderer(renderer), Panel("Material Editor") {
        m_open = false;
        frameBufferID = renderer.createFramebuffer(1080, 720);
    }

    void MaterialEditor::init() {
        m_sphere.init();
        m_sphere.getTransform().position = glm::vec3(0, 0, 0);
        m_camera.getTransform().position = glm::vec3(0, 0, 3);
    }

    void MaterialEditor::onUpdate() {
        //Set flags for being able to close the viewport
        if (m_open) {
            if (ImGui::Begin("Material Editor", &m_open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                if (ImGui::BeginTable("Content", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable)) {
                    ImGui::TableSetupColumn("Outline", 0, 250.f);
                    ImGui::TableSetupColumn("Editor", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);

                    ImGui::BeginChild("Settings");
                    ImGui::Text("Material Settings");
                    ImGui::Separator();
                    ImGui::Text("Name: %s", m_material->getName().c_str());
                    ImGui::Separator();
                    ImGui::Text("Color");
                    ImGui::SameLine();
                    ImGui::ColorEdit4("##Color", glm::value_ptr(m_material->getColor()));
                    ImGui::Separator();
                    ImGuiUtils::drawVec3Control("Ambient", m_material->getAmbient(), 1, 100.0f, 0, 1);
                    ImGuiUtils::drawVec3Control("diffuse", m_material->getDiffuse(), 1, 100.0f, 0, 1);
                    ImGuiUtils::drawVec3Control("specular", m_material->getSpecular(), 1, 100.0f, 0, 1);
                    ImGui::Separator();

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
                ImGui::End();
                if (!m_open) {
                    MaterialManager::serializeMaterial(m_material->getName(), m_filepath);
                    for (GameObject *gameObject: Scene::getInstance().getAllGameObjects()) {
                        if (gameObject->hasComponent<MeshRendererComponent>()) {
                            MeshRendererComponent *meshRendererComponent = gameObject->getComponent<MeshRendererComponent>();
                            if (meshRendererComponent->getMaterial().getName() == m_material->getName()) {
                                meshRendererComponent->paintMesh();
                            }
                        }
                    }
                }
            }
        }
    }

    void MaterialEditor::renderObjectWithMaterial() {
        CameraComponent *currentMainCamera = SceneHelper::mainCamera;
        if (!currentMainCamera) {
            return;
        }
        m_camera.getComponent<CameraComponent>()->update();
        SceneHelper::changeMainCameraTo(m_camera.getComponent<CameraComponent>());
        m_sphere.getComponent<MeshRendererComponent>()->changeMaterial(*m_material);
        FrameBuffer &frameBuffer = m_renderer.getFramebuffer(frameBufferID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
        ImGui::Begin(name.c_str());
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        frameBuffer.bind();
        frameBuffer.resize(wsize.x, wsize.y);
        std::vector<GameObject *> objects = {&m_sphere, &m_camera};
        m_renderer.renderCustomPipeline(objects);
        uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
        ImGui::Image(reinterpret_cast<void *>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        frameBuffer.unBind();
        ImGui::End();
        ImGui::PopStyleVar();
        SceneHelper::changeMainCameraTo(currentMainCamera);
    }

    void MaterialEditor::open(const std::string &name, const std::string &filepath) {
        m_open = true;
        m_material = &MaterialManager::getMaterial(name);
        m_sphere.getComponent<MeshRendererComponent>()->changeMaterial(*m_material);
        m_filepath = filepath;
    }
}