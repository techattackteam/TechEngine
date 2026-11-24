#pragma once

#include <TechEngine/core/resources/ResourceId.hpp>

#include <cstdint>
#include <string>
#include <variant>

namespace TechEngine {
    enum class FieldKind : std::uint8_t { Bool, Int8, UInt8, Int16, UInt16, Int32, UInt32, Int64, UInt64, Float, Double, String, StringId, Asset, Entity, Bulk };

    // A read-only kind holds what the inspector shows: a StringId's hash and a bulk field's element count as
    // std::uint64_t, and an Entity's target name as std::string. An asset field holds its ResourceId.
    using FieldValue = std::variant<bool, std::int8_t, std::uint8_t, std::int16_t, std::uint16_t, std::int32_t, std::uint32_t, std::int64_t, std::uint64_t, float, double, std::string, ResourceId>;

    std::string describeFieldValue(const FieldValue& value);
}
