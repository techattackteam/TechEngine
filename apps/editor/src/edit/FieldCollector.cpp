#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Name.hpp>

#include <edit/FieldCollector.hpp>

#include <utility>

namespace TechEngine {
    FieldCollector::FieldCollector(const Scene& scene, std::vector<FieldRow>& rows) : m_scene(&scene), m_rows(&rows) {
    }

    void FieldCollector::visitBool(const std::string_view name, bool& value) {
        appendRow(name, FieldKind::Bool, value);
    }

    void FieldCollector::visitInt8(const std::string_view name, std::int8_t& value) {
        appendRow(name, FieldKind::Int8, value);
    }

    void FieldCollector::visitUInt8(const std::string_view name, std::uint8_t& value) {
        appendRow(name, FieldKind::UInt8, value);
    }

    void FieldCollector::visitInt16(const std::string_view name, std::int16_t& value) {
        appendRow(name, FieldKind::Int16, value);
    }

    void FieldCollector::visitUInt16(const std::string_view name, std::uint16_t& value) {
        appendRow(name, FieldKind::UInt16, value);
    }

    void FieldCollector::visitInt32(const std::string_view name, std::int32_t& value) {
        appendRow(name, FieldKind::Int32, value);
    }

    void FieldCollector::visitUInt32(const std::string_view name, std::uint32_t& value) {
        appendRow(name, FieldKind::UInt32, value);
    }

    void FieldCollector::visitInt64(const std::string_view name, std::int64_t& value) {
        appendRow(name, FieldKind::Int64, value);
    }

    void FieldCollector::visitUInt64(const std::string_view name, std::uint64_t& value) {
        appendRow(name, FieldKind::UInt64, value);
    }

    void FieldCollector::visitFloat(const std::string_view name, float& value) {
        appendRow(name, FieldKind::Float, value);
    }

    void FieldCollector::visitDouble(const std::string_view name, double& value) {
        appendRow(name, FieldKind::Double, value);
    }

    void FieldCollector::visitString(const std::string_view name, std::string& value) {
        appendRow(name, FieldKind::String, value);
    }

    void FieldCollector::visitStringId(const std::string_view name, const StringId value) {
        appendRow(name, FieldKind::StringId, value.value());
    }

    void FieldCollector::visitEntity(const std::string_view name, const Entity value) {
        const std::string targetName = m_scene->contains(value) ? m_scene->getComponent<Name>(value).value : std::string();
        appendRow(name, FieldKind::Entity, targetName);
    }

    void FieldCollector::visitAsset(const std::string_view name, const ResourceId id) {
        appendRow(name, FieldKind::Asset, id.value());
    }

    void FieldCollector::visitBulk(const std::string_view name, const std::size_t count) {
        appendRow(name, FieldKind::Bulk, static_cast<std::uint64_t>(count));
    }

    void FieldCollector::appendRow(const std::string_view name, const FieldKind kind, FieldValue value) {
        FieldRow& row = m_rows->emplace_back();
        row.index = m_index;
        row.name = name;
        row.kind = kind;
        row.value = std::move(value);
        m_index++;
    }
}
