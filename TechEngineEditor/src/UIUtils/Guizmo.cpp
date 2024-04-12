#include "Guizmo.hpp"
#include <glm/gtx/matrix_decompose.hpp>

#include "components/physics/BoxColliderComponent.hpp"
#include "components/physics/SphereCollider.hpp"
#include "panels/PanelsManager.hpp"

namespace TechEngine {
    Guizmo::Guizmo(PhysicsEngine& physicsEngine) : physicsEngine(physicsEngine) {
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

    void Guizmo::editTransform(CameraComponent* camera, ImGuiContext* context, std::vector<GameObject*>& selectedGameObjects) {
        if (operation == -1) {
            return;
        }
        if (selectedGameObjects.empty() /*|| !SceneHelper::hasMainCamera()*/) {
            return;
        }

        ImGuizmo::SetImGuiContext(context);
        ImGuizmo::SetOrthographic(false);
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

        glm::mat4 transform = selectedGameObjects.front()->getModelMatrix();
        Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                   (ImGuizmo::OPERATION)operation, (ImGuizmo::MODE)(mode), glm::value_ptr(transform),
                   nullptr, nullptr);

        if (ImGuizmo::IsUsing()) {
            glm::vec3 translation, rotation, scale;
            DecomposeTransform(transform, translation, rotation, scale);
            selectedGameObjects.front()->getTransform().translateToWorld(translation);
            selectedGameObjects.front()->getTransform().setRotation(glm::degrees(rotation));
            selectedGameObjects.front()->getTransform().setScale(scale);
            if (selectedGameObjects.front()->hasComponent<RigidBody>()) {
                physicsEngine.addRigidBody(selectedGameObjects.front()->getComponent<RigidBody>());
            } else if (selectedGameObjects.front()->hasComponent<BoxColliderComponent>()) {
                physicsEngine.addCollider(selectedGameObjects.front()->getComponent<BoxColliderComponent>());
            } else if (selectedGameObjects.front()->hasComponent<SphereCollider>()) {
                physicsEngine.addCollider(selectedGameObjects.front()->getComponent<SphereCollider>());
            } else if (selectedGameObjects.front()->hasComponent<CylinderCollider>()) {
                physicsEngine.addCollider(selectedGameObjects.front()->getComponent<CylinderCollider>());
            }
        }
    }

    void Guizmo::setOperation(int operation) {
        this->operation = operation;
    }

    int Guizmo::getMode() const {
        return mode;
    }

    void Guizmo::setMode(int mode) {
        this->mode = mode;
    }
}
