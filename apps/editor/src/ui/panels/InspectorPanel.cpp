#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <edit/EditorCommands.hpp>
#include <edit/EditorSnapshot.hpp>
#include <edit/FieldValue.hpp>
#include <ui/panels/InspectorPanel.hpp>
#include <ui/widgets/DragControls.hpp>
#include <ui/widgets/PropertyTable.hpp>

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <variant>

namespace TechEngine {
    static constexpr const char* COMPONENT_MENU = "InspectorComponentMenu";
    static constexpr const char* ADD_COMPONENT_MENU = "InspectorAddComponentMenu";
    static constexpr std::array<const char*, 4> FIXED_LABELS = {"Translation", "Rotation", "Scale", "Mesh"};
    // Half of the 0.01 that "%.2f" shows: anything closer to zero would print as -0.00.
    static constexpr float EULER_DISPLAY_EPSILON = 0.005f;
    static constexpr float FLOAT_DRAG_SPEED = 0.1f;
    static constexpr float INTEGER_DRAG_SPEED = 1.0f;

    // Positions in Transform's and MeshRenderer's visit, so reordering either visit moves them.
    static constexpr std::uint32_t TRANSFORM_POSITION_FIELD = 0;
    static constexpr std::uint32_t TRANSFORM_ROTATION_FIELD = 3;
    static constexpr std::uint32_t TRANSFORM_SCALE_FIELD = 7;
    static constexpr std::size_t TRANSFORM_FIELD_COUNT = 10;
    static constexpr std::size_t MESH_RENDERER_MESH_FIELD = 0;

    template<typename T>
    static ImGuiDataType toImGuiDataType() {
        if constexpr (std::is_same_v<T, std::int8_t>) {
            return ImGuiDataType_S8;
        } else if constexpr (std::is_same_v<T, std::uint8_t>) {
            return ImGuiDataType_U8;
        } else if constexpr (std::is_same_v<T, std::int16_t>) {
            return ImGuiDataType_S16;
        } else if constexpr (std::is_same_v<T, std::uint16_t>) {
            return ImGuiDataType_U16;
        } else if constexpr (std::is_same_v<T, std::int32_t>) {
            return ImGuiDataType_S32;
        } else if constexpr (std::is_same_v<T, std::uint32_t>) {
            return ImGuiDataType_U32;
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
            return ImGuiDataType_S64;
        } else if constexpr (std::is_same_v<T, std::uint64_t>) {
            return ImGuiDataType_U64;
        } else if constexpr (std::is_same_v<T, float>) {
            return ImGuiDataType_Float;
        } else {
            static_assert(std::is_same_v<T, double>, "No ImGui data type for this field type");
            return ImGuiDataType_Double;
        }
    }

    InspectorPanel::InspectorPanel(EditorCommands& commands) : Panel("Inspector"), m_commands(&commands) {
    }

    void InspectorPanel::onDraw(const EditorSnapshot& snapshot) {
        if (!snapshot.selected.valid()) {
            return;
        }
        if (snapshot.selected != m_textFieldsEntity) {
            m_textFields.clear();
            m_textFieldsEntity = snapshot.selected;
        }

        m_labelWidth = measureLabelWidth(snapshot);
        drawName(snapshot);
        for (const ComponentRow& component: snapshot.components) {
            drawComponent(snapshot.selected, component);
        }
        drawAddMenu(snapshot);
    }

    float InspectorPanel::measureLabelWidth(const EditorSnapshot& snapshot) const {
        float width = 0.0f;
        for (const char* label: FIXED_LABELS) {
            width = std::max(width, ImGui::CalcTextSize(label).x);
        }
        for (const ComponentRow& component: snapshot.components) {
            for (const FieldRow& field: component.fields) {
                width = std::max(width, ImGui::CalcTextSize(field.name.c_str()).x);
            }
        }
        // The table already pads every cell, so the column needs only the text.
        return width;
    }

    void InspectorPanel::drawName(const EditorSnapshot& snapshot) {
        std::size_t row = 0;
        const bool found = snapshot.findRow(snapshot.selected, row);
        if (!found) {
            return;
        }

        const std::string& name = snapshot.hierarchy[row].name;
        // The selection can change while a name is being typed, so the rename goes to the entity the edit started on.
        if (!m_nameField.isEditing()) {
            m_nameEntity = snapshot.selected;
        }
        ImGui::SetNextItemWidth(-FLT_MIN);
        std::string edited;
        const bool finished = m_nameField.draw("##EntityName", name, edited);
        if (finished && edited != name) {
            m_commands->rename(m_nameEntity, edited);
        }
        ImGui::Separator();
    }

    void InspectorPanel::drawComponent(const Entity entity, const ComponentRow& component) {
        ImGui::PushID(component.tag.c_str());
        const float rightX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
        // NoTreePushOnOpen keeps the body at the header's left edge instead of one indent in, and needs no TreePop.
        const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        const std::string_view name = getDisplayName(component.tag);
        const bool open = ImGui::TreeNodeEx("##component", flags, "%.*s", static_cast<int>(name.size()), name.data());
        if (component.removable) {
            drawComponentMenu(entity, component, rightX);
        }

        if (open) {
            if (beginPropertyTable("##properties", m_labelWidth)) {
                if (component.type == componentTypeId<Transform>()) {
                    drawTransform(entity, component);
                } else if (component.type == componentTypeId<MeshRenderer>()) {
                    drawMeshRenderer(component);
                } else {
                    drawFields(entity, component);
                }
                endPropertyTable();
            }
        }
        ImGui::PopID();
    }

    void InspectorPanel::drawComponentMenu(const Entity entity, const ComponentRow& component, const float rightX) {
        const float buttonSize = ImGui::GetFrameHeight();
        ImGui::SameLine(rightX - buttonSize);
        if (ImGui::Button("-", ImVec2(buttonSize, buttonSize))) {
            ImGui::OpenPopup(COMPONENT_MENU);
        }
        if (!ImGui::BeginPopup(COMPONENT_MENU)) {
            return;
        }
        if (ImGui::MenuItem("Remove Component")) {
            m_commands->removeComponent(entity, component.type);
        }
        ImGui::EndPopup();
    }

    void InspectorPanel::drawTransform(const Entity entity, const ComponentRow& component) {
        std::array<float, TRANSFORM_FIELD_COUNT> values{};
        if (component.fields.size() != TRANSFORM_FIELD_COUNT) {
            drawFields(entity, component);
            return;
        }
        for (std::size_t i = 0; i < TRANSFORM_FIELD_COUNT; i++) {
            const float* value = std::get_if<float>(&component.fields[i].value);
            if (value == nullptr) {
                drawFields(entity, component);
                return;
            }
            values[i] = *value;
        }

        std::array<bool, 3> changedAxes{};
        Vec3 position(values[0], values[1], values[2]);
        propertyRow("Translation");
        if (drawVec3Control("##translation", position, 0.0f, changedAxes)) {
            sendVec3(entity, TRANSFORM_POSITION_FIELD, position, changedAxes);
        }

        syncEuler(entity, Quat(values[3], values[4], values[5], values[6]));
        Vec3 euler = m_euler;
        propertyRow("Rotation");
        if (drawVec3Control("##rotation", euler, 0.0f, changedAxes)) {
            m_euler = euler;
            m_eulerRotation = Transform::fromEulerDegrees(euler);
            m_rotationPending = true;
            // Every component of the quaternion moves with any Euler angle, and the snapshot may still hold an older one, so all four are sent.
            const std::array<float, 4> rotation = {m_eulerRotation.w, m_eulerRotation.x, m_eulerRotation.y, m_eulerRotation.z};
            for (std::uint32_t i = 0; i < rotation.size(); i++) {
                m_commands->setField(entity, componentTypeId<Transform>(), TRANSFORM_ROTATION_FIELD + i, FieldValue{rotation[i]});
            }
        }

        Vec3 scale(values[7], values[8], values[9]);
        propertyRow("Scale");
        if (drawVec3Control("##scale", scale, 1.0f, changedAxes)) {
            sendVec3(entity, TRANSFORM_SCALE_FIELD, scale, changedAxes);
        }
    }

    void InspectorPanel::drawMeshRenderer(const ComponentRow& component) {
        const std::uint64_t* mesh = nullptr;
        if (component.fields.size() > MESH_RENDERER_MESH_FIELD) {
            mesh = std::get_if<std::uint64_t>(&component.fields[MESH_RENDERER_MESH_FIELD].value);
        }

        propertyRow("Mesh");
        if (mesh == nullptr || *mesh == 0) {
            ImGui::TextDisabled("None");
        } else {
            ImGui::TextDisabled("%016llx", static_cast<unsigned long long>(*mesh));
        }
    }

    void InspectorPanel::drawFields(const Entity entity, const ComponentRow& component) {
        for (const FieldRow& field: component.fields) {
            drawField(entity, component.type, field);
        }
    }

    void InspectorPanel::drawField(const Entity entity, const ComponentTypeId component, const FieldRow& field) {
        ImGui::PushID(static_cast<int>(field.index));
        propertyRow(field.name.c_str());
        ImGui::SetNextItemWidth(-FLT_MIN);

        switch (field.kind) {
            case FieldKind::Bool: {
                const bool* shown = std::get_if<bool>(&field.value);
                bool value = shown != nullptr && *shown;
                if (ImGui::Checkbox("##value", &value)) {
                    m_commands->setField(entity, component, field.index, FieldValue{value});
                }
                break;
            }
            case FieldKind::Int8:
            case FieldKind::UInt8:
            case FieldKind::Int16:
            case FieldKind::UInt16:
            case FieldKind::Int32:
            case FieldKind::UInt32:
            case FieldKind::Int64:
            case FieldKind::UInt64:
            case FieldKind::Float:
            case FieldKind::Double: {
                std::visit(
                    [&](auto value) {
                        using T = decltype(value);
                        if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>) {
                            const float speed = std::is_floating_point_v<T> ? FLOAT_DRAG_SPEED : INTEGER_DRAG_SPEED;
                            if (dragScalar("##value", toImGuiDataType<T>(), &value, speed)) {
                                m_commands->setField(entity, component, field.index, FieldValue{value});
                            }
                        }
                    },
                    field.value);
                break;
            }
            case FieldKind::String: {
                const std::string* text = std::get_if<std::string>(&field.value);
                if (text == nullptr) {
                    break;
                }
                TextEditField& textField = m_textFields[ImGui::GetID("##value")];
                std::string edited;
                const bool finished = textField.draw("##value", *text, edited);
                if (finished && edited != *text) {
                    m_commands->setField(entity, component, field.index, FieldValue{edited});
                }
                break;
            }
            case FieldKind::StringId:
            case FieldKind::Asset: {
                const std::uint64_t* value = std::get_if<std::uint64_t>(&field.value);
                ImGui::TextDisabled("%016llx", static_cast<unsigned long long>(value != nullptr ? *value : 0));
                break;
            }
            case FieldKind::Entity: {
                const std::string* target = std::get_if<std::string>(&field.value);
                ImGui::TextDisabled("%s", target != nullptr && !target->empty() ? target->c_str() : "None");
                break;
            }
            case FieldKind::Bulk: {
                const std::uint64_t* count = std::get_if<std::uint64_t>(&field.value);
                ImGui::TextDisabled("%llu elements", static_cast<unsigned long long>(count != nullptr ? *count : 0));
                break;
            }
        }
        ImGui::PopID();
    }

    void InspectorPanel::drawAddMenu(const EditorSnapshot& snapshot) {
        ImGui::Spacing();
        ImGui::BeginDisabled(snapshot.addable.empty());
        if (ImGui::Button("Add Component", ImVec2(-FLT_MIN, 0.0f))) {
            ImGui::OpenPopup(ADD_COMPONENT_MENU);
        }
        ImGui::EndDisabled();

        if (!ImGui::BeginPopup(ADD_COMPONENT_MENU)) {
            return;
        }
        for (const ComponentChoice& choice: snapshot.addable) {
            ImGui::PushID(choice.tag.c_str());
            const std::string name(getDisplayName(choice.tag));
            if (ImGui::MenuItem(name.c_str())) {
                m_commands->addComponent(snapshot.selected, choice.type);
            }
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }

    void InspectorPanel::syncEuler(const Entity entity, const Quat& rotation) {
        if (entity != m_eulerEntity) {
            m_eulerEntity = entity;
            m_eulerRotation = rotation;
            m_euler = roundNearZero(Transform::toEulerDegrees(rotation));
            m_rotationPending = false;
            return;
        }
        // Until the snapshot shows the rotation that was sent, an older one is only the simulation lagging behind.
        if (m_rotationPending) {
            m_rotationPending = rotation != m_eulerRotation;
            return;
        }
        if (rotation != m_eulerRotation) {
            m_eulerRotation = rotation;
            m_euler = roundNearZero(Transform::toEulerDegrees(rotation));
        }
    }

    void InspectorPanel::sendVec3(const Entity entity, const std::uint32_t firstField, const Vec3& values, const std::array<bool, 3>& changedAxes) {
        for (std::uint32_t axis = 0; axis < changedAxes.size(); axis++) {
            if (changedAxes[axis]) {
                m_commands->setField(entity, componentTypeId<Transform>(), firstField + axis, FieldValue{values[static_cast<int>(axis)]});
            }
        }
    }

    Vec3 InspectorPanel::roundNearZero(const Vec3& angles) {
        Vec3 rounded = angles;
        for (int axis = 0; axis < 3; axis++) {
            if (std::abs(rounded[axis]) < EULER_DISPLAY_EPSILON) {
                rounded[axis] = 0.0f;
            }
        }
        return rounded;
    }

    std::string_view InspectorPanel::getDisplayName(const std::string_view tag) {
        const std::size_t separator = tag.rfind("::");
        if (separator == std::string_view::npos) {
            return tag;
        }
        return tag.substr(separator + 2);
    }
}
