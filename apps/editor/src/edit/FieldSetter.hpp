#pragma once

#include <TechEngine/core/serialization/FieldVisitor.hpp>

#include <edit/FieldValue.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace TechEngine {
    class FieldSetter final : public FieldVisitor {
    private:
        std::uint32_t m_target = 0;
        const FieldValue* m_value = nullptr;
        std::uint32_t m_index = 0;
        FieldValue m_oldValue;
        bool m_applied = false;

    public:
        // value must outlive the walk.
        FieldSetter(std::uint32_t field, const FieldValue& value);

        // False when the index names no field, a read-only field, or a field whose type differs from the value's.
        bool wasApplied() const;

        const FieldValue& getOldValue() const;

        void visitBool(std::string_view name, bool& value) override;

        void visitInt8(std::string_view name, std::int8_t& value) override;

        void visitUInt8(std::string_view name, std::uint8_t& value) override;

        void visitInt16(std::string_view name, std::int16_t& value) override;

        void visitUInt16(std::string_view name, std::uint16_t& value) override;

        void visitInt32(std::string_view name, std::int32_t& value) override;

        void visitUInt32(std::string_view name, std::uint32_t& value) override;

        void visitInt64(std::string_view name, std::int64_t& value) override;

        void visitUInt64(std::string_view name, std::uint64_t& value) override;

        void visitFloat(std::string_view name, float& value) override;

        void visitDouble(std::string_view name, double& value) override;

        void visitString(std::string_view name, std::string& value) override;

        void visitStringId(std::string_view name, StringId value) override;

        void visitEntity(std::string_view name, Entity value) override;

        void visitAsset(std::string_view name, ResourceId id) override;

        void visitBulk(std::string_view name, std::size_t count) override;

    private:
        template<typename T>
        void assign(T& field);

        void skipReadOnly();
    };
}
