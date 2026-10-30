#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/serialization/FieldVisitor.hpp>
#include <TechEngine/core/serialization/Visit.hpp>

#include <serialization/FieldRecorder.hpp>

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

using TechEngine::AssetRef;
using TechEngine::Entity;
using TechEngine::FieldVisitor;
using TechEngine::ResourceHandle;
using TechEngine::ResourceId;
using TechEngine::StringId;
using TechEngineTests::FieldRecorder;
using TechEngineTests::RecordedField;

struct FieldVisitorAsset {};

struct FieldVisitorSample {
    bool enabled = true;
    std::int8_t int8 = -8;
    std::uint8_t uint8 = 8;
    std::int16_t int16 = -16;
    std::uint16_t uint16 = 16;
    std::int32_t int32 = -32;
    std::uint32_t uint32 = 32;
    std::int64_t int64 = -64;
    std::uint64_t uint64 = 64;
    float single = 1.5f;
    double precise = 2.5;
    std::string label = "Sample";
    StringId group = StringId("Group");
    Entity target = Entity{4, 2};
    AssetRef<FieldVisitorAsset> asset;
    std::vector<std::uint32_t> indices = {1, 2, 3};

    template<typename Archive>
    friend void visit(Archive& archive, FieldVisitorSample& value) {
        archive.field("enabled", value.enabled);
        archive.field("int8", value.int8);
        archive.field("uint8", value.uint8);
        archive.field("int16", value.int16);
        archive.field("uint16", value.uint16);
        archive.field("int32", value.int32);
        archive.field("uint32", value.uint32);
        archive.field("int64", value.int64);
        archive.field("uint64", value.uint64);
        archive.field("single", value.single);
        archive.field("precise", value.precise);
        archive.field("label", value.label);
        archive.field("group", value.group);
        archive.field("target", value.target);
        archive.field("asset", value.asset);
        archive.field("indices", value.indices);
    }
};

struct FieldVisitorInner {
    std::uint16_t count = 0;
    float scale = 1.0f;

    template<typename Archive>
    friend void visit(Archive& archive, FieldVisitorInner& value) {
        archive.field("count", value.count);
        archive.field("scale", value.scale);
    }
};

struct FieldVisitorOuter {
    bool before = false;
    FieldVisitorInner inner;
    std::string after;

    template<typename Archive>
    friend void visit(Archive& archive, FieldVisitorOuter& value) {
        archive.field("before", value.before);
        archive.field("inner", value.inner);
        archive.field("after", value.after);
    }
};

template<typename T>
concept NamelessField = requires(FieldVisitor& visitor, T& value) { visitor.field(value); };

static constexpr ResourceId FIELD_VISITOR_ASSET_ID{0xF1E1D0000000A55E};

static std::vector<std::string> kindsOf(const std::vector<RecordedField>& fields) {
    std::vector<std::string> kinds;
    for (const RecordedField& field: fields) {
        kinds.push_back(field.kind);
    }
    return kinds;
}

TEST_CASE("a type with a templated visit can be walked by the field visitor", "[core][serialization]") {
    STATIC_REQUIRE(TechEngine::Visitable<FieldVisitorSample, FieldVisitor>);
    STATIC_REQUIRE(TechEngine::Visitable<FieldVisitorOuter, FieldVisitor>);
}

TEST_CASE("the field visitor has no nameless field, so a visit must name every field", "[core][serialization]") {
    STATIC_REQUIRE_FALSE(NamelessField<std::uint32_t>);
    STATIC_REQUIRE_FALSE(NamelessField<std::string>);
    STATIC_REQUIRE_FALSE(NamelessField<Entity>);
    STATIC_REQUIRE_FALSE(NamelessField<FieldVisitorSample>);
}

TEST_CASE("the field visitor sees every field once, in visit order, with its name and its kind", "[core][serialization]") {
    FieldVisitorSample sample;
    FieldRecorder recorder;

    visit(recorder, sample);

    const std::vector<RecordedField> expected{
        {"enabled", "bool"},
        {"int8", "int8"},
        {"uint8", "uint8"},
        {"int16", "int16"},
        {"uint16", "uint16"},
        {"int32", "int32"},
        {"uint32", "uint32"},
        {"int64", "int64"},
        {"uint64", "uint64"},
        {"single", "float"},
        {"precise", "double"},
        {"label", "string"},
        {"group", "stringId"},
        {"target", "entity"},
        {"asset", "asset"},
        {"indices", "bulk"},
    };
    CHECK(recorder.fields == expected);
}

TEST_CASE("an empty Entity and an empty AssetRef are still visited, so the fields after them keep their positions", "[core][serialization]") {
    FieldVisitorSample sample;
    sample.target = Entity{};
    sample.asset = AssetRef<FieldVisitorAsset>{};
    FieldRecorder recorder;

    visit(recorder, sample);

    REQUIRE(recorder.fields.size() == 16);
    CHECK(recorder.fields[13] == RecordedField{"target", "entity"});
    CHECK(recorder.fields[14] == RecordedField{"asset", "asset"});
    CHECK(recorder.fields[15] == RecordedField{"indices", "bulk"});
    CHECK(recorder.entities == std::vector<Entity>{Entity{}});
    CHECK(recorder.assets == std::vector<ResourceId>{ResourceId{}});
}

TEST_CASE("an empty vector is still visited, with a count of zero", "[core][serialization]") {
    FieldVisitorSample sample;
    sample.indices.clear();
    FieldRecorder recorder;

    visit(recorder, sample);

    CHECK(recorder.bulkCounts == std::vector<std::size_t>{0});
}

TEST_CASE("a nested visited type's fields are walked in place, between their neighbours", "[core][serialization]") {
    FieldVisitorOuter outer;
    FieldRecorder recorder;

    visit(recorder, outer);

    const std::vector<std::string> expected{"bool", "uint16", "float", "string"};
    CHECK(kindsOf(recorder.fields) == expected);
}

TEST_CASE("the read-only kinds hand over the value they hold", "[core][serialization]") {
    FieldVisitorSample sample;
    sample.asset.id = FIELD_VISITOR_ASSET_ID;
    FieldRecorder recorder;

    visit(recorder, sample);

    CHECK(recorder.stringIds == std::vector<StringId>{StringId("Group")});
    CHECK(recorder.entities == std::vector<Entity>{Entity{4, 2}});
    CHECK(recorder.assets == std::vector<ResourceId>{FIELD_VISITOR_ASSET_ID});
    CHECK(recorder.bulkCounts == std::vector<std::size_t>{3});
}

TEST_CASE("the read-only kinds are passed by value, so no field visitor can change them", "[core][serialization]") {
    STATIC_REQUIRE(std::same_as<decltype(&FieldVisitor::visitStringId), void (FieldVisitor::*)(std::string_view, StringId)>);
    STATIC_REQUIRE(std::same_as<decltype(&FieldVisitor::visitEntity), void (FieldVisitor::*)(std::string_view, Entity)>);
    STATIC_REQUIRE(std::same_as<decltype(&FieldVisitor::visitAsset), void (FieldVisitor::*)(std::string_view, ResourceId)>);
    STATIC_REQUIRE(std::same_as<decltype(&FieldVisitor::visitBulk), void (FieldVisitor::*)(std::string_view, std::size_t)>);
}

TEST_CASE("a write through an editable field changes that field and no other", "[core][serialization]") {
    FieldVisitorSample sample;
    FieldRecorder recorder;
    recorder.writeName = "int32";
    recorder.writeValue = 42.0;

    visit(recorder, sample);

    const FieldVisitorSample untouched;
    CHECK(sample.int32 == 42);
    CHECK(sample.int8 == untouched.int8);
    CHECK(sample.uint32 == untouched.uint32);
    CHECK(sample.int64 == untouched.int64);
    CHECK(sample.single == untouched.single);
    CHECK(sample.precise == untouched.precise);
    CHECK(sample.label == untouched.label);
    CHECK(sample.indices == untouched.indices);
}
