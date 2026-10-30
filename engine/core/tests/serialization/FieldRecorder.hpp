#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/serialization/FieldVisitor.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace TechEngineTests {
    struct RecordedField {
        std::string name;
        std::string kind;

        bool operator==(const RecordedField&) const = default;
    };

    // Records every field it is shown, and sets a numeric field named writeName to writeValue.
    class FieldRecorder final : public TechEngine::FieldVisitor {
    public:
        std::vector<RecordedField> fields;
        std::vector<TechEngine::StringId> stringIds;
        std::vector<TechEngine::Entity> entities;
        std::vector<TechEngine::ResourceId> assets;
        std::vector<std::size_t> bulkCounts;
        std::string writeName;
        double writeValue = 0.0;

        void visitBool(const std::string_view name, bool&) override {
            record(name, "bool");
        }

        void visitInt8(const std::string_view name, std::int8_t& value) override {
            record(name, "int8");
            write(name, value);
        }

        void visitUInt8(const std::string_view name, std::uint8_t& value) override {
            record(name, "uint8");
            write(name, value);
        }

        void visitInt16(const std::string_view name, std::int16_t& value) override {
            record(name, "int16");
            write(name, value);
        }

        void visitUInt16(const std::string_view name, std::uint16_t& value) override {
            record(name, "uint16");
            write(name, value);
        }

        void visitInt32(const std::string_view name, std::int32_t& value) override {
            record(name, "int32");
            write(name, value);
        }

        void visitUInt32(const std::string_view name, std::uint32_t& value) override {
            record(name, "uint32");
            write(name, value);
        }

        void visitInt64(const std::string_view name, std::int64_t& value) override {
            record(name, "int64");
            write(name, value);
        }

        void visitUInt64(const std::string_view name, std::uint64_t& value) override {
            record(name, "uint64");
            write(name, value);
        }

        void visitFloat(const std::string_view name, float& value) override {
            record(name, "float");
            write(name, value);
        }

        void visitDouble(const std::string_view name, double& value) override {
            record(name, "double");
            write(name, value);
        }

        void visitString(const std::string_view name, std::string&) override {
            record(name, "string");
        }

        void visitStringId(const std::string_view name, const TechEngine::StringId value) override {
            record(name, "stringId");
            stringIds.push_back(value);
        }

        void visitEntity(const std::string_view name, const TechEngine::Entity value) override {
            record(name, "entity");
            entities.push_back(value);
        }

        void visitAsset(const std::string_view name, const TechEngine::ResourceId id) override {
            record(name, "asset");
            assets.push_back(id);
        }

        void visitBulk(const std::string_view name, const std::size_t count) override {
            record(name, "bulk");
            bulkCounts.push_back(count);
        }

    private:
        void record(const std::string_view name, const std::string_view kind) {
            fields.push_back(RecordedField{std::string(name), std::string(kind)});
        }

        template<typename T>
        void write(const std::string_view name, T& value) {
            if (!writeName.empty() && name == writeName) {
                value = static_cast<T>(writeValue);
            }
        }
    };
}
