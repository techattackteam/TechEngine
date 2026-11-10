#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <assets/AssetIndex.hpp>
#include <edit/EditorCommands.hpp>
#include <edit/EditorSnapshot.hpp>
#include <edit/FieldValue.hpp>
#include <edit/InspectorCommands.hpp>
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
#include <format>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace TechEngine {
    static constexpr const char* COMPONENT_MENU = "InspectorComponentMenu";
    static constexpr const char* ADD_COMPONENT_MENU = "InspectorAddComponentMenu";
    static constexpr std::array<const char*, 6> FIXED_LABELS = {"Mode", "Translation", "Rotation", "Scale", "Mesh", "Materials"};
    // Half of the 0.01 that "%.2f" shows: anything closer to zero would print as -0.00.
    static constexpr float EULER_DISPLAY_EPSILON = 0.005f;
    // A world rotation comes back as parent times a decomposed local one, so it never matches the one sent exactly.
    static constexpr float WORLD_ROTATION_TOLERANCE = 1e-4f;
    static constexpr float FLOAT_DRAG_SPEED = 0.1f;
    static constexpr float INTEGER_DRAG_SPEED = 1.0f;

    // Positions in Transform's and MeshRenderer's visit, so reordering either visit moves them.
    static constexpr std::uint32_t TRANSFORM_POSITION_FIELD = 0;
    static constexpr std::uint32_t TRANSFORM_ROTATION_FIELD = 3;
    static constexpr std::uint32_t TRANSFORM_SCALE_FIELD = 7;
    static constexpr std::size_t TRANSFORM_FIELD_COUNT = 10;
    static constexpr std::size_t MESH_RENDERER_MESH_FIELD = 0;
    static constexpr std::size_t MESH_RENDERER_FIRST_SLOT_FIELD = 1;
    static constexpr std::size_t MESH_RENDERER_SLOT_FIELD_COUNT = 2;

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
        if (!snapshot.active.valid()) {
            return;
        }
        if (snapshot.active != m_textFieldsEntity) {
            m_textFields.clear();
            m_textFieldsEntity = snapshot.active;
        }

        m_labelWidth = measureLabelWidth(snapshot);
        drawName(snapshot);
        for (const ComponentRow& component: snapshot.components) {
            drawComponent(snapshot, component);
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
        const bool found = snapshot.findRow(snapshot.active, row);
        if (!found) {
            return;
        }

        const std::string& name = snapshot.hierarchy[row].name;
        // The selection can change while a name is being typed, so the rename goes to the entities the edit started on.
        if (!m_nameField.isEditing()) {
            m_nameEntities = snapshot.selection;
        }
        const bool mixed = snapshot.hasMixedNames();
        ImGui::SetNextItemWidth(-FLT_MIN);
        std::string edited;
        const bool finished = m_nameField.draw("##EntityName", name, edited, mixed);
        if (finished && (mixed || edited != name)) {
            m_commands->rename(m_nameEntities, edited);
        }
        ImGui::Separator();
    }

    void InspectorPanel::drawComponent(const EditorSnapshot& snapshot, const ComponentRow& component) {
        ImGui::PushID(component.tag.c_str());
        const float rightX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
        // NoTreePushOnOpen keeps the body at the header's left edge instead of one indent in, and needs no TreePop.
        const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        const std::string_view name = getDisplayName(component.tag);
        const bool open = ImGui::TreeNodeEx("##component", flags, "%.*s", static_cast<int>(name.size()), name.data());
        if (component.removable) {
            drawComponentMenu(snapshot, component, rightX);
        }

        if (open) {
            if (beginPropertyTable("##properties", m_labelWidth)) {
                if (component.type == componentTypeId<Transform>()) {
                    drawTransform(snapshot, component);
                } else if (component.type == componentTypeId<MeshRenderer>()) {
                    drawMeshRenderer(snapshot, component);
                } else {
                    drawFields(snapshot, component);
                }
                endPropertyTable();
            }
        }
        ImGui::PopID();
    }

    void InspectorPanel::drawComponentMenu(const EditorSnapshot& snapshot, const ComponentRow& component, const float rightX) {
        const float buttonSize = ImGui::GetFrameHeight();
        ImGui::SameLine(rightX - buttonSize);
        if (ImGui::Button("-", ImVec2(buttonSize, buttonSize))) {
            ImGui::OpenPopup(COMPONENT_MENU);
        }
        if (!ImGui::BeginPopup(COMPONENT_MENU)) {
            return;
        }
        if (ImGui::MenuItem("Remove Component")) {
            m_commands->removeComponent(snapshot.selection, component.type);
        }
        ImGui::EndPopup();
    }

    void InspectorPanel::drawTransform(const EditorSnapshot& snapshot, const ComponentRow& component) {
        std::array<float, TRANSFORM_FIELD_COUNT> values{};
        if (component.fields.size() != TRANSFORM_FIELD_COUNT) {
            drawFields(snapshot, component);
            return;
        }
        for (std::size_t i = 0; i < TRANSFORM_FIELD_COUNT; i++) {
            const float* value = std::get_if<float>(&component.fields[i].value);
            if (value == nullptr) {
                drawFields(snapshot, component);
                return;
            }
            values[i] = *value;
        }

        propertyRow("Mode");
        drawModeToggle();

        TransformRow shown = snapshot.world;
        if (!m_worldMode) {
            shown.position = Vec3(values[TRANSFORM_POSITION_FIELD], values[TRANSFORM_POSITION_FIELD + 1], values[TRANSFORM_POSITION_FIELD + 2]);
            shown.rotation = Quat(values[TRANSFORM_ROTATION_FIELD], values[TRANSFORM_ROTATION_FIELD + 1], values[TRANSFORM_ROTATION_FIELD + 2], values[TRANSFORM_ROTATION_FIELD + 3]);
            shown.scale = Vec3(values[TRANSFORM_SCALE_FIELD], values[TRANSFORM_SCALE_FIELD + 1], values[TRANSFORM_SCALE_FIELD + 2]);
            shown.positionMixed = getMixedAxes(component, TRANSFORM_POSITION_FIELD);
            shown.rotationAxesMixed = snapshot.rotationAxesMixed;
            shown.scaleMixed = getMixedAxes(component, TRANSFORM_SCALE_FIELD);
        }

        std::array<bool, 3> changedAxes{};
        Vec3 position = shown.position;
        propertyRow("Translation");
        if (drawVec3Control("##translation", position, 0.0f, shown.positionMixed, changedAxes)) {
            sendAxes(snapshot, TransformPart::Position, position, changedAxes);
        }

        syncEuler(snapshot.active, shown.rotation);
        Vec3 euler = m_euler;
        propertyRow("Rotation");
        if (drawVec3Control("##rotation", euler, 0.0f, shown.rotationAxesMixed, changedAxes)) {
            m_euler = euler;
            m_eulerRotation = Transform::fromEulerDegrees(euler);
            m_rotationPending = true;
            sendAxes(snapshot, TransformPart::Rotation, euler, changedAxes);
        }

        Vec3 scale = shown.scale;
        propertyRow("Scale");
        if (drawVec3Control("##scale", scale, 1.0f, shown.scaleMixed, changedAxes)) {
            sendAxes(snapshot, TransformPart::Scale, scale, changedAxes);
        }
    }

    void InspectorPanel::drawModeToggle() {
        bool worldMode = m_worldMode;
        if (ImGui::RadioButton("Local", !worldMode)) {
            worldMode = false;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("World", worldMode)) {
            worldMode = true;
        }
        if (worldMode != m_worldMode) {
            m_worldMode = worldMode;
            // The Euler cache holds the other mode's angles, so the next sync takes this mode's rotation afresh.
            m_eulerEntity = Entity();
        }
    }

    void InspectorPanel::drawMeshRenderer(const EditorSnapshot& snapshot, const ComponentRow& component) {
        if (!hasMeshRendererRows(component)) {
            drawFields(snapshot, component);
            return;
        }

        const FieldRow& mesh = component.fields[MESH_RENDERER_MESH_FIELD];
        ImGui::PushID(static_cast<int>(mesh.index));
        propertyRow("Mesh");
        ImGui::SetNextItemWidth(-FLT_MIN);
        drawAssetPicker(snapshot, component.type, mesh);
        ImGui::PopID();

        const std::size_t slotCount = (component.fields.size() - MESH_RENDERER_FIRST_SLOT_FIELD) / MESH_RENDERER_SLOT_FIELD_COUNT;
        bool slotsDiffer = component.fieldCountMixed;
        for (std::size_t slot = 0; slot < slotCount; slot++) {
            slotsDiffer = slotsDiffer || component.fields[MESH_RENDERER_FIRST_SLOT_FIELD + slot * MESH_RENDERER_SLOT_FIELD_COUNT].mixed;
        }
        if (slotsDiffer) {
            propertyRow("Materials");
            ImGui::TextDisabled("Slots differ across the selection");
            return;
        }

        for (std::size_t slot = 0; slot < slotCount; slot++) {
            const std::size_t nameField = MESH_RENDERER_FIRST_SLOT_FIELD + slot * MESH_RENDERER_SLOT_FIELD_COUNT;
            const std::string* name = std::get_if<std::string>(&component.fields[nameField].value);
            const FieldRow& material = component.fields[nameField + 1];
            ImGui::PushID(static_cast<int>(material.index));
            propertyRow(name != nullptr ? name->c_str() : "");
            ImGui::SetNextItemWidth(-FLT_MIN);
            drawAssetPicker(snapshot, component.type, material);
            ImGui::PopID();
        }
    }

    bool InspectorPanel::hasMeshRendererRows(const ComponentRow& component) {
        if (component.fields.size() <= MESH_RENDERER_MESH_FIELD || component.fields[MESH_RENDERER_MESH_FIELD].kind != FieldKind::Asset) {
            return false;
        }
        if ((component.fields.size() - MESH_RENDERER_FIRST_SLOT_FIELD) % MESH_RENDERER_SLOT_FIELD_COUNT != 0) {
            return false;
        }
        for (std::size_t i = MESH_RENDERER_FIRST_SLOT_FIELD; i < component.fields.size(); i += MESH_RENDERER_SLOT_FIELD_COUNT) {
            if (component.fields[i].kind != FieldKind::String || component.fields[i + 1].kind != FieldKind::Asset) {
                return false;
            }
        }
        return true;
    }

    void InspectorPanel::drawFields(const EditorSnapshot& snapshot, const ComponentRow& component) {
        for (const FieldRow& field: component.fields) {
            drawField(snapshot, component.type, field);
        }
    }

    void InspectorPanel::drawField(const EditorSnapshot& snapshot, const ComponentTypeId component, const FieldRow& field) {
        ImGui::PushID(static_cast<int>(field.index));
        propertyRow(field.name.c_str());
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, field.mixed);

        switch (field.kind) {
            case FieldKind::Bool: {
                const bool* shown = std::get_if<bool>(&field.value);
                bool value = shown != nullptr && *shown;
                if (ImGui::Checkbox("##value", &value)) {
                    // A click on a mixed box would otherwise flip the active entity's value, so it checks every entity instead.
                    m_commands->setField(snapshot.selection, component, field.index, FieldValue{field.mixed || value});
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
                                m_commands->setField(snapshot.selection, component, field.index, FieldValue{value});
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
                const bool finished = textField.draw("##value", *text, edited, field.mixed);
                if (finished && (field.mixed || edited != *text)) {
                    m_commands->setField(snapshot.selection, component, field.index, FieldValue{edited});
                }
                break;
            }
            case FieldKind::Asset:
                drawAssetPicker(snapshot, component, field);
                break;
            case FieldKind::StringId:
            case FieldKind::Entity:
            case FieldKind::Bulk:
                drawReadOnlyField(field);
                break;
        }
        ImGui::PopItemFlag();
        ImGui::PopID();
    }

    void InspectorPanel::drawAssetPicker(const EditorSnapshot& snapshot, const ComponentTypeId component, const FieldRow& field) {
        const ResourceId* shown = std::get_if<ResourceId>(&field.value);
        const ResourceId current = shown != nullptr ? *shown : ResourceId{};

        std::string preview;
        if (field.mixed) {
            preview = "-";
        } else if (!current.valid()) {
            preview = "None";
        } else {
            const auto record = std::ranges::find(snapshot.assets, current, &AssetRecord::id);
            preview = record != snapshot.assets.end() ? record->path : std::format("{0:016x}", current.value());
        }

        if (!ImGui::BeginCombo("##asset", preview.c_str())) {
            return;
        }
        snapshot.collectAssetsOfType(field.assetType, m_assetChoices);
        if (m_assetChoices.empty()) {
            ImGui::TextDisabled("No assets of this type");
        }
        for (const std::size_t choice: m_assetChoices) {
            const AssetRecord& record = snapshot.assets[choice];
            const bool selected = !field.mixed && record.id == current;
            if (ImGui::Selectable(record.path.c_str(), selected) && !selected) {
                m_commands->setAssetField(snapshot.selection, component, field.index, record.id);
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    void InspectorPanel::drawReadOnlyField(const FieldRow& field) {
        if (field.mixed) {
            ImGui::TextDisabled("-");
            return;
        }

        switch (field.kind) {
            case FieldKind::StringId: {
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
            default:
                break;
        }
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
                m_commands->addComponent(snapshot.selection, choice.type);
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
            if (isSentRotation(rotation)) {
                m_rotationPending = false;
                m_eulerRotation = rotation;
            }
            return;
        }
        if (rotation != m_eulerRotation) {
            m_eulerRotation = rotation;
            m_euler = roundNearZero(Transform::toEulerDegrees(rotation));
        }
    }

    bool InspectorPanel::isSentRotation(const Quat& rotation) const {
        if (!m_worldMode) {
            return rotation == m_eulerRotation;
        }
        // q and -q are the same rotation.
        const float sign = glm::dot(rotation, m_eulerRotation) < 0.0f ? -1.0f : 1.0f;
        for (int i = 0; i < 4; i++) {
            if (std::abs(rotation[i] - sign * m_eulerRotation[i]) > WORLD_ROTATION_TOLERANCE) {
                return false;
            }
        }
        return true;
    }

    void InspectorPanel::sendAxes(const EditorSnapshot& snapshot, const TransformPart part, const Vec3& values, const std::array<bool, 3>& changedAxes) {
        for (std::uint32_t axis = 0; axis < changedAxes.size(); axis++) {
            if (!changedAxes[axis]) {
                continue;
            }
            const float value = values[static_cast<int>(axis)];
            if (m_worldMode) {
                m_commands->setWorldAxis(snapshot.selection, snapshot.active, m_eulerRotation, part, axis, value);
                continue;
            }
            switch (part) {
                case TransformPart::Position:
                    m_commands->setField(snapshot.selection, componentTypeId<Transform>(), TRANSFORM_POSITION_FIELD + axis, FieldValue{value});
                    break;
                case TransformPart::Rotation:
                    m_commands->setRotationAxis(snapshot.selection, snapshot.active, m_eulerRotation, axis, value);
                    break;
                case TransformPart::Scale:
                    m_commands->setField(snapshot.selection, componentTypeId<Transform>(), TRANSFORM_SCALE_FIELD + axis, FieldValue{value});
                    break;
            }
        }
    }

    std::array<bool, 3> InspectorPanel::getMixedAxes(const ComponentRow& component, const std::uint32_t firstField) {
        std::array<bool, 3> mixed{};
        for (std::size_t axis = 0; axis < mixed.size(); axis++) {
            mixed[axis] = component.fields[firstField + axis].mixed;
        }
        return mixed;
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
