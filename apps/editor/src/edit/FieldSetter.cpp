#include <edit/FieldSetter.hpp>

#include <variant>

namespace TechEngine {
    FieldSetter::FieldSetter(const std::uint32_t field, const FieldValue& value) : m_target(field), m_value(&value) {
    }

    bool FieldSetter::wasApplied() const {
        return m_applied;
    }

    const FieldValue& FieldSetter::getOldValue() const {
        return m_oldValue;
    }

    template<typename T>
    void FieldSetter::assign(T& field) {
        if (m_target == m_index) {
            const T* value = std::get_if<T>(m_value);
            if (value != nullptr) {
                m_oldValue = field;
                field = *value;
                m_applied = true;
            }
        }
        m_index++;
    }

    void FieldSetter::skipReadOnly() {
        m_index++;
    }

    void FieldSetter::visitBool(std::string_view, bool& value) {
        assign(value);
    }

    void FieldSetter::visitInt8(std::string_view, std::int8_t& value) {
        assign(value);
    }

    void FieldSetter::visitUInt8(std::string_view, std::uint8_t& value) {
        assign(value);
    }

    void FieldSetter::visitInt16(std::string_view, std::int16_t& value) {
        assign(value);
    }

    void FieldSetter::visitUInt16(std::string_view, std::uint16_t& value) {
        assign(value);
    }

    void FieldSetter::visitInt32(std::string_view, std::int32_t& value) {
        assign(value);
    }

    void FieldSetter::visitUInt32(std::string_view, std::uint32_t& value) {
        assign(value);
    }

    void FieldSetter::visitInt64(std::string_view, std::int64_t& value) {
        assign(value);
    }

    void FieldSetter::visitUInt64(std::string_view, std::uint64_t& value) {
        assign(value);
    }

    void FieldSetter::visitFloat(std::string_view, float& value) {
        assign(value);
    }

    void FieldSetter::visitDouble(std::string_view, double& value) {
        assign(value);
    }

    void FieldSetter::visitString(std::string_view, std::string& value) {
        assign(value);
    }

    void FieldSetter::visitStringId(std::string_view, StringId) {
        skipReadOnly();
    }

    void FieldSetter::visitEntity(std::string_view, Entity) {
        skipReadOnly();
    }

    void FieldSetter::visitAsset(std::string_view, ResourceId) {
        skipReadOnly();
    }

    void FieldSetter::visitBulk(std::string_view, std::size_t) {
        skipReadOnly();
    }
}
