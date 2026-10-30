#pragma once

#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <ui/panels/Panel.hpp>
#include <ui/widgets/TextEditField.hpp>

#include <imgui.h>

#include <array>
#include <cstdint>
#include <string_view>
#include <unordered_map>

namespace TechEngine {
    class EditorCommands;
    struct ComponentRow;
    struct FieldRow;

    class InspectorPanel final : public Panel {
    private:
        EditorCommands* m_commands = nullptr;
        float m_labelWidth = 0.0f;
        TextEditField m_nameField;
        Entity m_nameEntity;
        std::unordered_map<ImGuiID, TextEditField> m_textFields;
        Entity m_textFieldsEntity;
        // Euler angles are kept across frames, because decomposing the quaternion each frame can flip
        // them while a drag moves through them.
        Entity m_eulerEntity;
        Vec3 m_euler = Vec3(0.0f);
        Quat m_eulerRotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
        bool m_rotationPending = false;

    public:
        explicit InspectorPanel(EditorCommands& commands);

    protected:
        void onDraw(const EditorSnapshot& snapshot) override;

    private:
        float measureLabelWidth(const EditorSnapshot& snapshot) const;

        void drawName(const EditorSnapshot& snapshot);

        void drawComponent(Entity entity, const ComponentRow& component);

        void drawComponentMenu(Entity entity, const ComponentRow& component, float rightX);

        void drawTransform(Entity entity, const ComponentRow& component);

        void drawMeshRenderer(const ComponentRow& component);

        void drawFields(Entity entity, const ComponentRow& component);

        void drawField(Entity entity, ComponentTypeId component, const FieldRow& field);

        void drawAddMenu(const EditorSnapshot& snapshot);

        void syncEuler(Entity entity, const Quat& rotation);

        void sendVec3(Entity entity, std::uint32_t firstField, const Vec3& values, const std::array<bool, 3>& changedAxes);

        static std::string_view getDisplayName(std::string_view tag);

        static Vec3 roundNearZero(const Vec3& angles);
    };
}
