#include "Guizmo.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "components/Archetype.hpp"
#include "physics/PhysicsEngine.hpp"
#include "../ui/Widget.hpp"
#include "scene/ScenesManager.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    Guizmo::Guizmo(int& id, SystemsRegistry& systemsRegistry) : id(id), m_systemsRegistry(systemsRegistry) {
    }

    bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale) {
        // From glm::decompose in matrix_decompose.inl

        using namespace glm;
        using T = float;

        mat4 LocalMatrix(transform);

        // Normalize the matrix.
        if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>()))
            return false;

        // First, isolate perspective.  This is the messiest.
        if (
            epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
            epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
            epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>())) {
            // Clear the perspective partition
            LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
            LocalMatrix[3][3] = static_cast<T>(1);
        }

        // Next take care of translation (easy).
        translation = vec3(LocalMatrix[3]);
        LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

        vec3 Row[3], Pdum3;

        // Now get scale and shear.
        for (length_t i = 0; i < 3; ++i)
            for (length_t j = 0; j < 3; ++j)
                Row[i][j] = LocalMatrix[i][j];

        // Compute X scale factor and normalize first row.
        scale.x = length(Row[0]);
        Row[0] = detail::scale(Row[0], static_cast<T>(1));
        scale.y = length(Row[1]);
        Row[1] = detail::scale(Row[1], static_cast<T>(1));
        scale.z = length(Row[2]);
        Row[2] = detail::scale(Row[2], static_cast<T>(1));

        // At this point, the matrix (in rows[]) is orthonormal.
        // Check for a coordinate system flip.  If the determinant
        // is -1, then negate the matrix and the scaling factors.

        rotation.y = asin(-Row[0][2]);
        if (cos(rotation.y) != 0) {
            rotation.x = atan2(Row[1][2], Row[2][2]);
            rotation.z = atan2(Row[0][1], Row[0][0]);
        } else {
            rotation.x = atan2(-Row[2][0], Row[1][1]);
            rotation.z = 0;
        }


        return true;
    }

    void Guizmo::editTransform(Camera* camera, ImGuiContext* context, const std::vector<Entity>& selectedEntities) {
        if (operation == -1) {
            return;
        }
        if (selectedEntities.empty()) {
            return;
        }

        ImGuizmo::SetImGuiContext(context);
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::AllowAxisFlip(false);
        ImGuizmo::SetDrawlist();
        auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
        auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
        auto viewportOffset = ImGui::GetWindowPos();
        glm::vec2 m_ViewportBounds[2];
        m_ViewportBounds[0] = {viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y};
        m_ViewportBounds[1] = {viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y};
        ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y, m_ViewportBounds[1].x - m_ViewportBounds[0].x, m_ViewportBounds[1].y - m_ViewportBounds[0].y);

        const glm::mat4& cameraProjection = camera->getProjectionMatrix();
        glm::mat4 cameraView = camera->getViewMatrix();
        Entity entity = selectedEntities.front();
        Transform& transform = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getComponent<Transform>(entity);
        glm::mat4 modelMatrix = transform.getModelMatrix();
        Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                   (ImGuizmo::OPERATION)operation, (ImGuizmo::MODE)(mode), glm::value_ptr(modelMatrix),
                   nullptr, nullptr);

        if (ImGuizmo::IsUsing() && (lastUsingID == -1 || lastUsingID == id)) {
            lastUsingID = id;
            glm::vec3 translation, rotation, scale;
            DecomposeTransform(modelMatrix, translation, rotation, scale);

            transform.translateToWorld(translation);
            transform.setRotation(degrees(rotation));
            transform.setScale(scale);
            Scene& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();
            m_systemsRegistry.getSystem<PhysicsEngine>().moveOrRotateBody(scene.getComponent<Tag>(entity), scene.getComponent<Transform>(entity));
        } else {
            lastUsingID = -1;
        }
    }

    void Guizmo::editUI(ImVec2 viewPosition, ImVec2 viewSize, Widget* widget) {
        /*if (widget == nullptr || operation == -1) {
            return;
        }
        Rml::Vector2f position;
        Rml::Vector2f size;
        std::vector<WidgetProperty> properties = widget->getProperties();

        for (const auto& property: properties) {
            if (property.name == "Position") {
                position = std::get<Rml::Vector2f>(property.value);
            } else if (property.name == "Size") {
                size = std::get<Rml::Vector2f>(property.value);
            }
        }

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(position.x, position.y, 0.0f));
        //modelMatrix = glm::scale(modelMatrix, glm::vec3(size.x, size.y, 1.0f));

        glm::mat4 viewMatrix = glm::mat4(1.0f);

        glm::mat4 projectionMatrix = glm::ortho(0.0f, viewSize.x, viewSize.y, 0.f, -1.0f, 1.0f);

        ImGuizmo::SetOrthographic(true);
        ImGuizmo::AllowAxisFlip(true);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

        ImGuizmo::SetRect(viewPosition.x, viewPosition.y, viewSize.x, viewSize.y);

        ImGuizmo::MODE currentMode = (operation == ImGuizmo::OPERATION::SCALE) ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD;


        TE_LOGGER_INFO("UI View Position: ({0}, {1}), Size: ({2}, {3})", viewPosition.x, viewPosition.y, viewSize.x, viewSize.y);
        ImGuizmo::Manipulate(glm::value_ptr(viewMatrix), glm::value_ptr(projectionMatrix),
                             (ImGuizmo::OPERATION)operation, currentMode, glm::value_ptr(modelMatrix),
                             nullptr, nullptr);
        if (ImGuizmo::IsUsing() && (lastUsingID == -1 || lastUsingID == id) && widget) {
            lastUsingID = id;
            glm::vec3 translation, rotation, scale;
            DecomposeTransform(modelMatrix, translation, rotation, scale);

            for (auto& property: widget->getProperties()) {
                if (property.name == "Position") {
                    auto& value = std::get<Rml::Vector2f>(property.value);
                    value.x = translation.x;
                    value.y = translation.y;
                    property.onChange(value);
                } else if (property.name == "Rotation") {
                    auto& value = std::get<Rml::Vector3f>(property.value);
                    value.x = rotation.x;
                    value.y = rotation.y;
                    value.z = rotation.z;
                    property.onChange(value);
                } else if (property.name == "Scale") {
                    auto& value = std::get<Rml::Vector2f>(property.value);
                    value.x = scale.x;
                    value.y = scale.y;
                    property.onChange(value);
                }
            }
        } else {
            lastUsingID = -1;
        }*/
    }

    void Guizmo::setOperation(int operation) {
        this->operation = operation;
    }

    ImGuizmo::MODE Guizmo::getMode() const {
        return static_cast<ImGuizmo::MODE>(mode);
    }

    void Guizmo::setMode(int mode) {
        this->mode = mode;
    }
}
