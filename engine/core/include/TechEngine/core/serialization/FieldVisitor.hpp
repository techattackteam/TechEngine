#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/serialization/Visit.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace TechEngine {
    // Unlike the other archives it has no nameless field(value), so a visit that forgets a name does not compile.
    class FieldVisitor {
    public:
        virtual ~FieldVisitor() = default;

        virtual void visitBool(std::string_view name, bool& value) = 0;

        virtual void visitInt8(std::string_view name, std::int8_t& value) = 0;

        virtual void visitUInt8(std::string_view name, std::uint8_t& value) = 0;

        virtual void visitInt16(std::string_view name, std::int16_t& value) = 0;

        virtual void visitUInt16(std::string_view name, std::uint16_t& value) = 0;

        virtual void visitInt32(std::string_view name, std::int32_t& value) = 0;

        virtual void visitUInt32(std::string_view name, std::uint32_t& value) = 0;

        virtual void visitInt64(std::string_view name, std::int64_t& value) = 0;

        virtual void visitUInt64(std::string_view name, std::uint64_t& value) = 0;

        virtual void visitFloat(std::string_view name, float& value) = 0;

        virtual void visitDouble(std::string_view name, double& value) = 0;

        virtual void visitString(std::string_view name, std::string& value) = 0;

        virtual void visitStringId(std::string_view name, StringId value) = 0;

        virtual void visitEntity(std::string_view name, Entity value) = 0;

        virtual void visitAsset(std::string_view name, ResourceId id) = 0;

        virtual void visitBulk(std::string_view name, std::size_t count) = 0;

        void field(const std::string_view name, bool& value) {
            visitBool(name, value);
        }

        void field(const std::string_view name, std::int8_t& value) {
            visitInt8(name, value);
        }

        void field(const std::string_view name, std::uint8_t& value) {
            visitUInt8(name, value);
        }

        void field(const std::string_view name, std::int16_t& value) {
            visitInt16(name, value);
        }

        void field(const std::string_view name, std::uint16_t& value) {
            visitUInt16(name, value);
        }

        void field(const std::string_view name, std::int32_t& value) {
            visitInt32(name, value);
        }

        void field(const std::string_view name, std::uint32_t& value) {
            visitUInt32(name, value);
        }

        void field(const std::string_view name, std::int64_t& value) {
            visitInt64(name, value);
        }

        void field(const std::string_view name, std::uint64_t& value) {
            visitUInt64(name, value);
        }

        void field(const std::string_view name, float& value) {
            visitFloat(name, value);
        }

        void field(const std::string_view name, double& value) {
            visitDouble(name, value);
        }

        void field(const std::string_view name, std::string& value) {
            visitString(name, value);
        }

        void field(const std::string_view name, StringId& value) {
            visitStringId(name, value);
        }

        void field(const std::string_view name, Entity& value) {
            visitEntity(name, value);
        }

        template<typename T>
        void field(const std::string_view name, AssetRef<T>& value) {
            visitAsset(name, value.id);
        }

        template<typename T>
        void field(const std::string_view name, std::vector<T>& values) {
            visitBulk(name, values.size());
        }

        template<typename T>
        void field(std::string_view, T& value) {
            static_assert(Visitable<T, FieldVisitor>, "No field overload and no visit(archive, value) for this type; give it a visit function in its own namespace.");
            visit(*this, value);
        }
    };
}
