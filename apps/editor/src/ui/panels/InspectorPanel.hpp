#pragma once

#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <ui/panels/Panel.hpp>
#include <ui/widgets/TextEditField.hpp>

#include <imgui.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace TechEngine {
    class EditorCommands;
    struct ComponentRow;
    struct FieldRow;
    enum class TransformPart : std::uint8_t;

    class InspectorPanel final : public Panel {
    private:
        EditorCommands* m_commands = nullptr;
        float m_labelWidth = 0.0f;
        TextEditField m_nameField;
        std::vector<Entity> m_nameEntities;
        std::unordered_map<ImGuiID, TextEditField> m_textFields;
        Entity m_textFieldsEntity;
        // Euler angles are kept across frames, because decomposing the quaternion each frame can flip
        // them while a drag moves through them.
        Entity m_eulerEntity;
        Vec3 m_euler = Vec3(0.0f);
        Quat m_eulerRotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
        bool m_rotationPending = false;
        bool m_worldMode = false;
        std::vector<std::size_t> m_assetChoices;

    public:
        explicit InspectorPanel(EditorCommands& commands);

    protected:
        void onDraw(const EditorSnapshot& snapshot) override;

    private:
        float measureLabelWidth(const EditorSnapshot& snapshot) const;

        void drawName(const EditorSnapshot& snapshot);

        void drawComponent(const EditorSnapshot& snapshot, const ComponentRow& component);

        void drawComponentMenu(const EditorSnapshot& snapshot, const ComponentRow& component, float rightX);

        void drawTransform(const EditorSnapshot& snapshot, const ComponentRow& component);

        void drawModeToggle();

        void drawMeshRenderer(const EditorSnapshot& snapshot, const ComponentRow& component);

        static bool hasMeshRendererRows(const ComponentRow& component);

        void drawFields(const EditorSnapshot& snapshot, const ComponentRow& component);

        void drawField(const EditorSnapshot& snapshot, ComponentTypeId component, const FieldRow& field);

        static void drawReadOnlyField(const FieldRow& field);

        void drawAssetPicker(const EditorSnapshot& snapshot, ComponentTypeId component, const FieldRow& field);

        void drawAddMenu(const EditorSnapshot& snapshot);

        void syncEuler(Entity entity, const Quat& rotation);

        bool isSentRotation(const Quat& rotation) const;

        void sendAxes(const EditorSnapshot& snapshot, TransformPart part, const Vec3& values, const std::array<bool, 3>& changedAxes);

        static std::array<bool, 3> getMixedAxes(const ComponentRow& component, std::uint32_t firstField);

        static std::string_view getDisplayName(std::string_view tag);

        static Vec3 roundNearZero(const Vec3& angles);
    };
}
