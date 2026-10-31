#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Name.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <edit/EditSession.hpp>
#include <edit/EditTestScene.hpp>
#include <edit/EditorSnapshot.hpp>
#include <edit/FieldValue.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

using TechEngine::ComponentRow;
using TechEngine::EditorSnapshot;
using TechEngine::Entity;
using TechEngine::FieldKind;
using TechEngine::FieldRow;
using TechEngine::FieldValue;
using TechEngine::HierarchyRow;
using TechEngine::Quat;
using TechEngine::Transform;
using TechEngine::Vec3;
using TechEngineTests::componentRowFor;
using TechEngineTests::EditTestScene;
using TechEngineTests::findComponentRow;
using TechEngineTests::InspectedSample;
using TechEngineTests::rowFor;
using TechEngineTests::rowIndex;
using TechEngineTests::SAMPLE_ASSET_FIELD;
using TechEngineTests::SAMPLE_COUNT_FIELD;
using TechEngineTests::SAMPLE_ENABLED_FIELD;
using TechEngineTests::SAMPLE_FIELD_COUNT;
using TechEngineTests::SAMPLE_FLOAT_FIELD;
using TechEngineTests::SAMPLE_GROUP_FIELD;
using TechEngineTests::SAMPLE_INDICES_FIELD;
using TechEngineTests::SAMPLE_INT16_FIELD;
using TechEngineTests::SAMPLE_INT64_FIELD;
using TechEngineTests::SAMPLE_INT8_FIELD;
using TechEngineTests::SAMPLE_LABEL_FIELD;
using TechEngineTests::SAMPLE_LAYER_FIELD;
using TechEngineTests::SAMPLE_TARGET_FIELD;
using TechEngineTests::SAMPLE_UINT16_FIELD;
using TechEngineTests::SAMPLE_UINT32_FIELD;
using TechEngineTests::SAMPLE_UINT64_FIELD;
using TechEngineTests::SAMPLE_WEIGHT_FIELD;
using TechEngineTests::TRANSFORM_FIELD_COUNT;

static void checkRow(const HierarchyRow& row, const Entity entity, const std::uint32_t depth, const std::string_view name, const std::size_t childCount) {
    CHECK(row.entity == entity);
    CHECK(row.depth == depth);
    CHECK(row.name == name);
    CHECK(row.childCount == childCount);
}

TEST_CASE("an empty scene publishes no rows, no selection and equal revisions", "[editor][edit]") {
    EditTestScene edit;

    const EditorSnapshot snapshot = edit.snapshot();

    CHECK(snapshot.hierarchy.empty());
    CHECK_FALSE(snapshot.selected.valid());
    CHECK(snapshot.editRevision == snapshot.savedRevision);
}

TEST_CASE("the snapshot lists the hierarchy in tree order with depth, name and child count", "[editor][edit]") {
    EditTestScene edit;
    const Entity a = edit.createNamed("A");
    const Entity b = edit.createNamed("B", a);
    const Entity d = edit.createNamed("D", b);
    const Entity c = edit.createNamed("C", a);
    const Entity e = edit.createNamed("E");

    const EditorSnapshot snapshot = edit.snapshot();

    REQUIRE(snapshot.hierarchy.size() == 5);
    // Roots come in storage order, so only each root's own run of rows is checked.
    const std::size_t first = rowIndex(snapshot, a);
    REQUIRE(first + 3 < snapshot.hierarchy.size());
    checkRow(snapshot.hierarchy[first], a, 0, "A", 2);
    checkRow(snapshot.hierarchy[first + 1], b, 1, "B", 1);
    checkRow(snapshot.hierarchy[first + 2], d, 2, "D", 0);
    checkRow(snapshot.hierarchy[first + 3], c, 1, "C", 0);
    checkRow(rowFor(snapshot, e), e, 0, "E", 0);
}

TEST_CASE("the snapshot publishes names as they are, empty and repeated", "[editor][edit]") {
    EditTestScene edit;
    const Entity first = edit.createNamed("Twin");
    const Entity second = edit.createNamed("Twin");
    const Entity unnamed = edit.createNamed("");
    const Entity untouched = edit.scene.createEntity();

    const EditorSnapshot snapshot = edit.snapshot();

    REQUIRE(snapshot.hierarchy.size() == 4);
    CHECK(rowFor(snapshot, first).name == "Twin");
    CHECK(rowFor(snapshot, second).name == "Twin");
    CHECK(rowFor(snapshot, unnamed).name.empty());
    CHECK(rowFor(snapshot, untouched).name == "Entity");
}

TEST_CASE("a chain of 1,000 nested entities publishes one row per depth, in order", "[editor][edit]") {
    EditTestScene edit;
    constexpr std::uint32_t CHAIN_LENGTH = 1000;
    std::vector<Entity> chain;
    chain.push_back(edit.createNamed("Link"));
    for (std::uint32_t i = 1; i < CHAIN_LENGTH; i++) {
        chain.push_back(edit.createNamed("Link", chain.back()));
    }

    const EditorSnapshot snapshot = edit.snapshot();

    REQUIRE(snapshot.hierarchy.size() == CHAIN_LENGTH);
    for (std::uint32_t i = 0; i < CHAIN_LENGTH; i++) {
        CHECK(snapshot.hierarchy[i].entity == chain[i]);
        CHECK(snapshot.hierarchy[i].depth == i);
        CHECK(snapshot.hierarchy[i].childCount == (i + 1 < CHAIN_LENGTH ? 1U : 0U));
    }
}

TEST_CASE("the snapshot carries the session's selection and both revisions", "[editor][edit]") {
    EditTestScene edit;
    const Entity a = edit.createNamed("A");
    const EditorSnapshot before = edit.snapshot();

    SECTION("a selection is published without a revision change") {
        edit.session.setSelected(a);

        const EditorSnapshot selected = edit.snapshot();

        CHECK(selected.selected == a);
        CHECK(selected.editRevision == before.editRevision);
        CHECK(selected.savedRevision == before.savedRevision);
    }

    SECTION("a recorded scene change raises only the edit revision") {
        edit.session.recordSceneChange();

        const EditorSnapshot changed = edit.snapshot();

        CHECK(changed.editRevision > before.editRevision);
        CHECK(changed.savedRevision == before.savedRevision);
        CHECK(changed.editRevision != changed.savedRevision);
        CHECK_FALSE(changed.selected.valid());
    }

    SECTION("an empty entity clears the selection") {
        edit.session.setSelected(a);
        edit.session.setSelected(Entity{});

        CHECK_FALSE(edit.snapshot().selected.valid());
    }
}

TEST_CASE("a recorded save makes the revisions equal until the next scene change", "[editor][edit][save]") {
    EditTestScene edit;
    CHECK_FALSE(edit.snapshot().hasUnsavedEdits());

    edit.session.recordSceneChange();
    edit.session.recordSceneChange();
    const EditorSnapshot edited = edit.snapshot();
    REQUIRE(edited.hasUnsavedEdits());

    edit.session.recordSave();
    const EditorSnapshot saved = edit.snapshot();
    CHECK(saved.savedRevision == saved.editRevision);
    CHECK(saved.editRevision == edited.editRevision);
    CHECK_FALSE(saved.hasUnsavedEdits());

    SECTION("a later scene change marks the scene unsaved again") {
        edit.session.recordSceneChange();

        const EditorSnapshot changed = edit.snapshot();
        CHECK(changed.savedRevision == saved.savedRevision);
        CHECK(changed.hasUnsavedEdits());
    }

    SECTION("a second save with nothing changed keeps the revisions") {
        edit.session.recordSave();

        const EditorSnapshot again = edit.snapshot();
        CHECK(again.editRevision == saved.editRevision);
        CHECK(again.savedRevision == saved.savedRevision);
    }

    SECTION("a selection after the save leaves it clean") {
        edit.session.setSelected(edit.createNamed("A"));

        CHECK_FALSE(edit.snapshot().hasUnsavedEdits());
    }
}

TEST_CASE("a snapshot is unsaved exactly while its revisions differ", "[editor][edit][save]") {
    EditorSnapshot snapshot;
    CHECK_FALSE(snapshot.hasUnsavedEdits());

    snapshot.editRevision = 3;
    snapshot.savedRevision = 2;
    CHECK(snapshot.hasUnsavedEdits());

    snapshot.savedRevision = 3;
    CHECK_FALSE(snapshot.hasUnsavedEdits());
}

TEST_CASE("a session has no scene file until one is set, then holds the last one set", "[editor][edit][save]") {
    TechEngine::EditSession session;
    TechEngine::SceneFile file;
    file.path = "unchanged";
    CHECK_FALSE(session.getSceneFile(file));
    CHECK(file.path == "unchanged");

    session.setSceneFile({TechEngine::ResourceId{0x11}, "project://assets/common/First.tescene"});
    session.setSceneFile({TechEngine::ResourceId{0x22}, "project://assets/common/Second.tescene"});

    REQUIRE(session.getSceneFile(file));
    CHECK(file.id == TechEngine::ResourceId{0x22});
    CHECK(file.path == "project://assets/common/Second.tescene");
}

TEST_CASE("building into a snapshot that already holds rows replaces them", "[editor][edit]") {
    EditTestScene edit;
    const Entity a = edit.createNamed("A");
    edit.createNamed("B", a);
    edit.session.setSelected(a);
    EditorSnapshot snapshot;
    edit.session.buildSnapshot(edit.scene, edit.getRegistry(), snapshot);

    edit.session.buildSnapshot(edit.scene, edit.getRegistry(), snapshot);

    REQUIRE(snapshot.hierarchy.size() == 2);
    CHECK(rowFor(snapshot, a).childCount == 1);
    REQUIRE(snapshot.components.size() == 1);
    CHECK(snapshot.components[0].fields.size() == TRANSFORM_FIELD_COUNT);
    CHECK(snapshot.addable.size() == 1);
}

TEST_CASE("with no selection the snapshot lists no components", "[editor][edit]") {
    EditTestScene edit;
    edit.createNamed("A");

    const EditorSnapshot snapshot = edit.snapshot();

    CHECK(snapshot.components.empty());
}

TEST_CASE("the selected entity's Transform is published as its ten local values in visit order", "[editor][edit]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("A");
    REQUIRE(edit.scene.getComponent<Transform>(entity).setLocal({Vec3(1.0f, 2.0f, 3.0f), Quat(1.0f, 0.0f, 0.0f, 0.0f), Vec3(4.0f, 5.0f, 6.0f)}));
    edit.session.setSelected(entity);

    const EditorSnapshot snapshot = edit.snapshot();

    const ComponentRow& transform = componentRowFor(snapshot, TechEngine::componentTypeId<Transform>());
    CHECK(transform.tag == Transform::tag);
    const std::vector<std::string> expectedNames{"position.x", "position.y", "position.z", "rotation.w", "rotation.x", "rotation.y", "rotation.z", "scale.x", "scale.y", "scale.z"};
    const std::vector<float> expectedValues{1.0f, 2.0f, 3.0f, 1.0f, 0.0f, 0.0f, 0.0f, 4.0f, 5.0f, 6.0f};
    REQUIRE(transform.fields.size() == TRANSFORM_FIELD_COUNT);
    for (std::size_t i = 0; i < transform.fields.size(); i++) {
        const FieldRow& field = transform.fields[i];
        CHECK(field.index == i);
        CHECK(field.name == expectedNames[i]);
        CHECK(field.kind == FieldKind::Float);
        REQUIRE(std::holds_alternative<float>(field.value));
        CHECK(std::get<float>(field.value) == expectedValues[i]);
    }
}

TEST_CASE("Name and Hierarchy never appear among the selected entity's components", "[editor][edit]") {
    EditTestScene edit;
    const Entity parent = edit.createNamed("Parent");
    const Entity child = edit.createNamed("Child", parent);
    edit.session.setSelected(child);

    const EditorSnapshot snapshot = edit.snapshot();

    CHECK(findComponentRow(snapshot, TechEngine::componentTypeId<TechEngine::Name>()) == nullptr);
    CHECK(findComponentRow(snapshot, TechEngine::componentTypeId<TechEngine::Hierarchy>()) == nullptr);
    CHECK(findComponentRow(snapshot, TechEngine::componentTypeId<Transform>()) != nullptr);
}

TEST_CASE("Transform is published as not removable and any other component as removable", "[editor][edit]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("A");
    edit.scene.addComponent<InspectedSample>(entity);
    edit.session.setSelected(entity);

    const EditorSnapshot snapshot = edit.snapshot();

    CHECK_FALSE(componentRowFor(snapshot, TechEngine::componentTypeId<Transform>()).removable);
    CHECK(componentRowFor(snapshot, TechEngine::componentTypeId<InspectedSample>()).removable);
}

TEST_CASE("the snapshot offers the registered types the selected entity lacks, never a built-in", "[editor][edit]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("A");
    edit.session.setSelected(entity);

    SECTION("an entity without the sample is offered the sample alone") {
        const EditorSnapshot snapshot = edit.snapshot();

        REQUIRE(snapshot.addable.size() == 1);
        CHECK(snapshot.addable[0].type == TechEngine::componentTypeId<InspectedSample>());
        CHECK(snapshot.addable[0].tag == InspectedSample::tag);
    }

    SECTION("an entity that holds the sample is offered nothing") {
        edit.scene.addComponent<InspectedSample>(entity);

        CHECK(edit.snapshot().addable.empty());
    }
}

TEST_CASE("each field row holds its kind and the value the inspector shows", "[editor][edit]") {
    EditTestScene edit;
    const Entity target = edit.createNamed("Target");
    const Entity entity = edit.createNamed("Holder");
    edit.scene.addComponent<InspectedSample>(entity);
    InspectedSample& sample = edit.scene.getComponent<InspectedSample>(entity);
    sample.target = target;
    sample.asset.id = TechEngine::ResourceId{0xA55E7};
    edit.session.setSelected(entity);

    const EditorSnapshot snapshot = edit.snapshot();

    const ComponentRow& row = componentRowFor(snapshot, TechEngine::componentTypeId<InspectedSample>());
    REQUIRE(row.fields.size() == SAMPLE_FIELD_COUNT);
    const auto checkField = [&](const std::uint32_t index, const std::string_view name, const FieldKind kind, const FieldValue& value) {
        const FieldRow& field = row.fields[index];
        CHECK(field.index == index);
        CHECK(field.name == name);
        CHECK(field.kind == kind);
        CHECK(field.value == value);
    };
    checkField(SAMPLE_ENABLED_FIELD, "enabled", FieldKind::Bool, FieldValue{true});
    checkField(SAMPLE_COUNT_FIELD, "count", FieldKind::Int32, FieldValue{std::int32_t{3}});
    checkField(SAMPLE_LAYER_FIELD, "layer", FieldKind::UInt8, FieldValue{std::uint8_t{1}});
    checkField(SAMPLE_WEIGHT_FIELD, "weight", FieldKind::Double, FieldValue{0.5});
    checkField(SAMPLE_LABEL_FIELD, "label", FieldKind::String, FieldValue{std::string("Sample")});
    checkField(SAMPLE_GROUP_FIELD, "group", FieldKind::StringId, FieldValue{TechEngine::StringId("Group").value()});
    checkField(SAMPLE_TARGET_FIELD, "target", FieldKind::Entity, FieldValue{std::string("Target")});
    checkField(SAMPLE_ASSET_FIELD, "asset", FieldKind::Asset, FieldValue{std::uint64_t{0xA55E7}});
    checkField(SAMPLE_INDICES_FIELD, "indices", FieldKind::Bulk, FieldValue{std::uint64_t{3}});
    checkField(SAMPLE_FLOAT_FIELD, "floatField", FieldKind::Float, FieldValue{1.5f});
    checkField(SAMPLE_INT8_FIELD, "int8Field", FieldKind::Int8, FieldValue{std::int8_t{-8}});
    checkField(SAMPLE_INT16_FIELD, "int16Field", FieldKind::Int16, FieldValue{std::int16_t{-16}});
    checkField(SAMPLE_UINT16_FIELD, "uint16Field", FieldKind::UInt16, FieldValue{std::uint16_t{16}});
    checkField(SAMPLE_UINT32_FIELD, "uint32Field", FieldKind::UInt32, FieldValue{std::uint32_t{32}});
    checkField(SAMPLE_INT64_FIELD, "int64Field", FieldKind::Int64, FieldValue{std::int64_t{-64}});
    checkField(SAMPLE_UINT64_FIELD, "uint64Field", FieldKind::UInt64, FieldValue{std::uint64_t{64}});
}

TEST_CASE("an Entity field with no live target is published with an empty name", "[editor][edit]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("Holder");
    edit.scene.addComponent<InspectedSample>(entity);
    edit.session.setSelected(entity);

    SECTION("the empty entity") {
        const EditorSnapshot snapshot = edit.snapshot();

        const FieldRow& target = componentRowFor(snapshot, TechEngine::componentTypeId<InspectedSample>()).fields[SAMPLE_TARGET_FIELD];
        CHECK(target.kind == FieldKind::Entity);
        CHECK(target.value == FieldValue{std::string()});
    }

    SECTION("a target that was destroyed") {
        const Entity target = edit.createNamed("Gone");
        edit.scene.getComponent<InspectedSample>(entity).target = target;
        REQUIRE(edit.scene.destroyEntity(target));

        const EditorSnapshot snapshot = edit.snapshot();

        const FieldRow& row = componentRowFor(snapshot, TechEngine::componentTypeId<InspectedSample>()).fields[SAMPLE_TARGET_FIELD];
        CHECK(row.value == FieldValue{std::string()});
    }
}

TEST_CASE("a selection whose entity is gone publishes no components and nothing to add", "[editor][edit]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("A");
    edit.session.setSelected(entity);
    REQUIRE(edit.scene.destroyEntity(entity));

    const EditorSnapshot snapshot = edit.snapshot();

    CHECK(snapshot.components.empty());
    CHECK(snapshot.addable.empty());
}
