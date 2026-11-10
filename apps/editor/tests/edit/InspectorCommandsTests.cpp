#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Name.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <edit/AssetEditHarness.hpp>
#include <edit/EditRecord.hpp>
#include <edit/EditTestScene.hpp>
#include <edit/EditorSnapshot.hpp>
#include <edit/FieldValue.hpp>
#include <edit/InspectorCommands.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using TechEngine::AddComponentCommand;
using TechEngine::ComponentAddedRecord;
using TechEngine::ComponentRemovedRecord;
using TechEngine::ComponentRow;
using TechEngine::ComponentTypeId;
using TechEngine::EditorSnapshot;
using TechEngine::EditRecord;
using TechEngine::Entity;
using TechEngine::FieldEditRecord;
using TechEngine::FieldValue;
using TechEngine::LocalTransformRecord;
using TechEngine::MeshRenderer;
using TechEngine::Quat;
using TechEngine::RemoveComponentCommand;
using TechEngine::ResourceId;
using TechEngine::ResourceState;
using TechEngine::RotationEditRecord;
using TechEngine::SetAssetFieldCommand;
using TechEngine::SetFieldCommand;
using TechEngine::SetRotationAxisCommand;
using TechEngine::SetWorldAxisCommand;
using TechEngine::Transform;
using TechEngine::TransformPart;
using TechEngine::TransformValues;
using TechEngine::Vec3;
using TechEngineTests::AssetEditHarness;
using TechEngineTests::componentRowFor;
using TechEngineTests::EDIT_BROKEN_MESH_ID;
using TechEngineTests::EDIT_MESH_ID;
using TechEngineTests::EDIT_OTHER_SCENE_ID;
using TechEngineTests::EDIT_SECOND_MESH_ID;
using TechEngineTests::EDIT_SERVER_MESH_ID;
using TechEngineTests::EDIT_UNKNOWN_ID;
using TechEngineTests::EditTestScene;
using TechEngineTests::findComponentRow;
using TechEngineTests::InspectedSample;
using TechEngineTests::MESH_RENDERER_MESH_FIELD;
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
using TechEngineTests::setEuler;
using TechEngineTests::TRANSFORM_FIELD_COUNT;
using TechEngineTests::TRANSFORM_POSITION_X_FIELD;
using TechEngineTests::TRANSFORM_POSITION_Y_FIELD;
using TechEngineTests::TRANSFORM_SCALE_X_FIELD;

static ComponentTypeId transformType() {
    return TechEngine::componentTypeId<Transform>();
}

static ComponentTypeId sampleType() {
    return TechEngine::componentTypeId<InspectedSample>();
}

static void applySetField(EditTestScene& edit, const Entity entity, const ComponentTypeId component, const std::uint32_t field, FieldValue value) {
    SetFieldCommand command{edit.session, edit.getRegistry(), {entity}, component, field, std::move(value)};
    command.apply(edit.scene);
}

TEST_CASE("set field changes exactly one field, publishes the new row and records the old value", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("A");
    REQUIRE(edit.scene.getComponent<Transform>(entity).setLocal({Vec3(1.0f, 2.0f, 3.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), Vec3(1.0f)}));
    edit.selectOnly(entity);
    const EditorSnapshot before = edit.snapshot();

    applySetField(edit, entity, transformType(), TRANSFORM_POSITION_X_FIELD, FieldValue{5.0f});

    const TransformValues& local = edit.scene.getComponent<Transform>(entity).getLocal();
    CHECK(local.position == Vec3(5.0f, 2.0f, 3.0f));
    CHECK(local.rotation == TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f));
    CHECK(local.scale == Vec3(1.0f));

    const EditorSnapshot after = edit.snapshot();
    CHECK(after.editRevision > before.editRevision);
    const std::vector<TechEngine::FieldRow>& beforeFields = componentRowFor(before, transformType()).fields;
    const std::vector<TechEngine::FieldRow>& afterFields = componentRowFor(after, transformType()).fields;
    REQUIRE(beforeFields.size() == TRANSFORM_FIELD_COUNT);
    REQUIRE(afterFields.size() == TRANSFORM_FIELD_COUNT);
    for (std::size_t i = 0; i < afterFields.size(); i++) {
        if (i == TRANSFORM_POSITION_X_FIELD) {
            CHECK(afterFields[i].value == FieldValue{5.0f});
        } else {
            CHECK(afterFields[i].value == beforeFields[i].value);
        }
    }

    REQUIRE(edit.session.getRecords().size() == 1);
    const EditRecord& record = edit.session.getRecords().back();
    REQUIRE(std::holds_alternative<FieldEditRecord>(record));
    const FieldEditRecord& fieldEdit = std::get<FieldEditRecord>(record);
    CHECK(fieldEdit.entity == entity);
    CHECK(fieldEdit.component == transformType());
    CHECK(fieldEdit.field == TRANSFORM_POSITION_X_FIELD);
    CHECK(fieldEdit.oldValue == FieldValue{1.0f});
}

TEST_CASE("a Transform field edit updates the world values of the entity and its children", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity parent = edit.createNamed("Parent");
    const Entity child = edit.createNamed("Child", parent);
    REQUIRE(edit.scene.getComponent<Transform>(child).setLocal({Vec3(0.0f, 2.0f, 0.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), Vec3(1.0f)}));

    applySetField(edit, parent, transformType(), TRANSFORM_POSITION_X_FIELD, FieldValue{5.0f});
    applySetField(edit, parent, transformType(), TRANSFORM_SCALE_X_FIELD, FieldValue{2.0f});

    const Vec3 parentWorld = edit.scene.getComponent<Transform>(parent).getWorld().position;
    const Vec3 childWorld = edit.scene.getComponent<Transform>(child).getWorld().position;
    CHECK(parentWorld.x == Catch::Approx(5.0f));
    CHECK(childWorld.x == Catch::Approx(5.0f));
    CHECK(childWorld.y == Catch::Approx(2.0f));
    CHECK(edit.scene.getComponent<Transform>(child).getWorld().scale.x == Catch::Approx(2.0f));
}

TEST_CASE("set field to the value the field already holds keeps the edit revision and records nothing", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("A");
    const EditorSnapshot before = edit.snapshot();

    applySetField(edit, entity, transformType(), TRANSFORM_POSITION_Y_FIELD, FieldValue{0.0f});

    CHECK(edit.snapshot().editRevision == before.editRevision);
    CHECK(edit.session.getRecords().empty());
}

TEST_CASE("set field writes every editable kind, including the fields after the read-only ones", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("A");
    edit.scene.addComponent<InspectedSample>(entity);
    edit.selectOnly(entity);

    struct FieldEdit {
        std::uint32_t field = 0;
        FieldValue value;
        FieldValue oldValue;
    };
    const std::vector<FieldEdit> edits = {
        {SAMPLE_ENABLED_FIELD, FieldValue{false}, FieldValue{true}},
        {SAMPLE_COUNT_FIELD, FieldValue{std::numeric_limits<std::int32_t>::min()}, FieldValue{std::int32_t{3}}},
        {SAMPLE_LAYER_FIELD, FieldValue{std::numeric_limits<std::uint8_t>::max()}, FieldValue{std::uint8_t{1}}},
        {SAMPLE_WEIGHT_FIELD, FieldValue{-2.25}, FieldValue{0.5}},
        {SAMPLE_LABEL_FIELD, FieldValue{std::string("Changed")}, FieldValue{std::string("Sample")}},
        {SAMPLE_FLOAT_FIELD, FieldValue{4.5f}, FieldValue{1.5f}},
        {SAMPLE_INT8_FIELD, FieldValue{std::numeric_limits<std::int8_t>::min()}, FieldValue{std::int8_t{-8}}},
        {SAMPLE_INT16_FIELD, FieldValue{std::numeric_limits<std::int16_t>::max()}, FieldValue{std::int16_t{-16}}},
        {SAMPLE_UINT16_FIELD, FieldValue{std::numeric_limits<std::uint16_t>::max()}, FieldValue{std::uint16_t{16}}},
        {SAMPLE_UINT32_FIELD, FieldValue{std::numeric_limits<std::uint32_t>::max()}, FieldValue{std::uint32_t{32}}},
        {SAMPLE_INT64_FIELD, FieldValue{std::numeric_limits<std::int64_t>::min()}, FieldValue{std::int64_t{-64}}},
        {SAMPLE_UINT64_FIELD, FieldValue{std::numeric_limits<std::uint64_t>::max()}, FieldValue{std::uint64_t{64}}},
    };

    for (const FieldEdit& fieldEdit: edits) {
        applySetField(edit, entity, sampleType(), fieldEdit.field, fieldEdit.value);
    }

    const InspectedSample& sample = edit.scene.getComponent<InspectedSample>(entity);
    CHECK_FALSE(sample.enabled);
    CHECK(sample.count == std::numeric_limits<std::int32_t>::min());
    CHECK(sample.layer == std::numeric_limits<std::uint8_t>::max());
    CHECK(sample.weight == -2.25);
    CHECK(sample.label == "Changed");
    CHECK(sample.floatField == 4.5f);
    CHECK(sample.int8Field == std::numeric_limits<std::int8_t>::min());
    CHECK(sample.int16Field == std::numeric_limits<std::int16_t>::max());
    CHECK(sample.uint16Field == std::numeric_limits<std::uint16_t>::max());
    CHECK(sample.uint32Field == std::numeric_limits<std::uint32_t>::max());
    CHECK(sample.int64Field == std::numeric_limits<std::int64_t>::min());
    CHECK(sample.uint64Field == std::numeric_limits<std::uint64_t>::max());

    const std::span<const EditRecord> records = edit.session.getRecords();
    REQUIRE(records.size() == edits.size());
    const EditorSnapshot after = edit.snapshot();
    const ComponentRow& row = componentRowFor(after, sampleType());
    for (std::size_t i = 0; i < edits.size(); i++) {
        const FieldEditRecord* record = std::get_if<FieldEditRecord>(&records[i]);
        REQUIRE(record != nullptr);
        CHECK(record->field == edits[i].field);
        CHECK(record->oldValue == edits[i].oldValue);
        CHECK(row.fields[edits[i].field].value == edits[i].value);
    }
}

TEST_CASE("set field refuses Name, which only a rename may change", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("Kept");
    const EditorSnapshot before = edit.snapshot();

    applySetField(edit, entity, TechEngine::componentTypeId<TechEngine::Name>(), 0, FieldValue{std::string("Changed")});

    CHECK(edit.scene.getComponent<TechEngine::Name>(entity).value == "Kept");
    CHECK(edit.session.getRecords().empty());
    CHECK(edit.snapshot().editRevision == before.editRevision);
}

TEST_CASE("set field, add and remove refuse a type the registry does not know, and record nothing", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("A");
    const ComponentTypeId unknown{TechEngine::StringId("Tests.Unregistered")};
    const EditorSnapshot before = edit.snapshot();

    SECTION("set field") {
        applySetField(edit, entity, unknown, 0, FieldValue{1.0f});
    }

    SECTION("add") {
        AddComponentCommand command{edit.session, {entity}, unknown};
        command.apply(edit.scene);
    }

    SECTION("remove") {
        RemoveComponentCommand command{edit.session, edit.getRegistry(), {entity}, unknown};
        command.apply(edit.scene);
    }

    CHECK(edit.scene.contains(entity));
    CHECK(edit.session.getRecords().empty());
    CHECK(edit.snapshot().editRevision == before.editRevision);
}

TEST_CASE("set field refuses a value whose type differs from the field's, and changes nothing", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("A");
    edit.scene.addComponent<InspectedSample>(entity);
    const EditorSnapshot before = edit.snapshot();

    SECTION("a double for a float field") {
        applySetField(edit, entity, transformType(), TRANSFORM_POSITION_X_FIELD, FieldValue{5.0});

        CHECK(edit.scene.getComponent<Transform>(entity).getLocal().position.x == 0.0f);
    }

    SECTION("an int64 for an int32 field") {
        applySetField(edit, entity, sampleType(), SAMPLE_COUNT_FIELD, FieldValue{std::int64_t{9}});

        CHECK(edit.scene.getComponent<InspectedSample>(entity).count == 3);
    }

    CHECK(edit.session.getRecords().empty());
    CHECK(edit.snapshot().editRevision == before.editRevision);
}

TEST_CASE("set field refuses every read-only field, even with a value of the type it is shown as", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity target = edit.createNamed("Target");
    const Entity entity = edit.createNamed("A");
    edit.scene.addComponent<InspectedSample>(entity);
    edit.scene.getComponent<InspectedSample>(entity).target = target;
    const InspectedSample before = edit.scene.getComponent<InspectedSample>(entity);
    const EditorSnapshot beforeSnapshot = edit.snapshot();

    applySetField(edit, entity, sampleType(), SAMPLE_GROUP_FIELD, FieldValue{std::uint64_t{1}});
    applySetField(edit, entity, sampleType(), SAMPLE_TARGET_FIELD, FieldValue{std::string("Other")});
    applySetField(edit, entity, sampleType(), SAMPLE_ASSET_FIELD, FieldValue{std::uint64_t{2}});
    applySetField(edit, entity, sampleType(), SAMPLE_ASSET_FIELD, FieldValue{TechEngine::ResourceId{2}});
    applySetField(edit, entity, sampleType(), SAMPLE_INDICES_FIELD, FieldValue{std::uint64_t{0}});

    const InspectedSample& after = edit.scene.getComponent<InspectedSample>(entity);
    CHECK(after.group == before.group);
    CHECK(after.target == target);
    CHECK(after.asset.id == before.asset.id);
    CHECK(after.indices == before.indices);
    CHECK(edit.session.getRecords().empty());
    CHECK(edit.snapshot().editRevision == beforeSnapshot.editRevision);
}

TEST_CASE("set field that names no live field changes nothing and records nothing", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("A");
    const EditorSnapshot before = edit.snapshot();

    SECTION("an index past the last field") {
        applySetField(edit, entity, transformType(), TRANSFORM_FIELD_COUNT, FieldValue{1.0f});
    }

    SECTION("a component the entity lacks") {
        applySetField(edit, entity, sampleType(), SAMPLE_COUNT_FIELD, FieldValue{std::int32_t{1}});

        CHECK_FALSE(edit.scene.hasComponent<InspectedSample>(entity));
    }

    SECTION("a destroyed entity") {
        const Entity dead = edit.createNamed("Dead");
        REQUIRE(edit.scene.destroyEntity(dead));

        applySetField(edit, dead, transformType(), TRANSFORM_POSITION_X_FIELD, FieldValue{1.0f});
    }

    SECTION("the empty entity") {
        applySetField(edit, Entity{}, transformType(), TRANSFORM_POSITION_X_FIELD, FieldValue{1.0f});
    }

    CHECK(edit.scene.getComponent<Transform>(entity).getLocal().position == Vec3(0.0f));
    CHECK(edit.session.getRecords().empty());
    CHECK(edit.snapshot().editRevision == before.editRevision);
}

TEST_CASE("add component gives the entity a default one, publishes it and records the add", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("A");
    edit.selectOnly(entity);
    const EditorSnapshot before = edit.snapshot();

    AddComponentCommand command{edit.session, {entity}, sampleType()};
    command.apply(edit.scene);

    REQUIRE(edit.scene.hasComponent<InspectedSample>(entity));
    CHECK(edit.scene.getComponent<InspectedSample>(entity).count == InspectedSample{}.count);
    const EditorSnapshot after = edit.snapshot();
    CHECK(after.editRevision > before.editRevision);
    CHECK(componentRowFor(after, sampleType()).fields.size() == SAMPLE_FIELD_COUNT);
    CHECK(after.addable.empty());

    REQUIRE(edit.session.getRecords().size() == 1);
    REQUIRE(std::holds_alternative<ComponentAddedRecord>(edit.session.getRecords()[0]));
    const ComponentAddedRecord& added = std::get<ComponentAddedRecord>(edit.session.getRecords()[0]);
    CHECK(added.entity == entity);
    CHECK(added.component == sampleType());
}

TEST_CASE("add component refuses a built-in, a type the entity holds and a dead entity, and records nothing", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("A");
    edit.scene.addComponent<InspectedSample>(entity);
    edit.scene.getComponent<InspectedSample>(entity).count = 11;
    const EditorSnapshot before = edit.snapshot();

    SECTION("Transform") {
        AddComponentCommand command{edit.session, {entity}, transformType()};
        command.apply(edit.scene);
    }

    SECTION("Name") {
        AddComponentCommand command{edit.session, {entity}, TechEngine::componentTypeId<TechEngine::Name>()};
        command.apply(edit.scene);
    }

    SECTION("a type the entity already holds") {
        AddComponentCommand command{edit.session, {entity}, sampleType()};
        command.apply(edit.scene);

        CHECK(edit.scene.getComponent<InspectedSample>(entity).count == 11);
    }

    SECTION("a destroyed entity") {
        const Entity dead = edit.createNamed("Dead");
        REQUIRE(edit.scene.destroyEntity(dead));

        AddComponentCommand command{edit.session, {dead}, sampleType()};
        command.apply(edit.scene);
    }

    CHECK(edit.session.getRecords().empty());
    CHECK(edit.snapshot().editRevision == before.editRevision);
}

TEST_CASE("remove component takes it off, records its payload and raises the edit revision", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("A");
    edit.scene.addComponent<InspectedSample>(entity);
    edit.scene.getComponent<InspectedSample>(entity).label = "Removed";
    edit.selectOnly(entity);
    const TechEngine::ComponentTypeRecord* typeRecord = edit.getRegistry().find(sampleType());
    REQUIRE(typeRecord != nullptr);
    std::vector<std::byte> expectedPayload;
    TechEngine::Writer writer{expectedPayload};
    typeRecord->writePayload(writer, &edit.scene.getComponent<InspectedSample>(entity));
    const EditorSnapshot before = edit.snapshot();

    RemoveComponentCommand command{edit.session, edit.getRegistry(), {entity}, sampleType()};
    command.apply(edit.scene);

    CHECK_FALSE(edit.scene.hasComponent<InspectedSample>(entity));
    const EditorSnapshot after = edit.snapshot();
    CHECK(after.editRevision > before.editRevision);
    CHECK(findComponentRow(after, sampleType()) == nullptr);
    REQUIRE(after.addable.size() == 1);
    CHECK(after.addable[0].type == sampleType());

    REQUIRE(edit.session.getRecords().size() == 1);
    REQUIRE(std::holds_alternative<ComponentRemovedRecord>(edit.session.getRecords()[0]));
    const ComponentRemovedRecord& removed = std::get<ComponentRemovedRecord>(edit.session.getRecords()[0]);
    CHECK(removed.entity == entity);
    CHECK(removed.component == sampleType());
    CHECK(removed.payload == expectedPayload);
}

TEST_CASE("remove component refuses the built-ins and a type the entity lacks, and records nothing", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity parent = edit.createNamed("Parent");
    const Entity entity = edit.createNamed("A", parent);
    const EditorSnapshot before = edit.snapshot();

    SECTION("Transform") {
        RemoveComponentCommand command{edit.session, edit.getRegistry(), {entity}, transformType()};
        command.apply(edit.scene);

        CHECK(edit.scene.hasComponent<Transform>(entity));
    }

    SECTION("Name") {
        RemoveComponentCommand command{edit.session, edit.getRegistry(), {entity}, TechEngine::componentTypeId<TechEngine::Name>()};
        command.apply(edit.scene);

        CHECK(edit.scene.getComponent<TechEngine::Name>(entity).value == "A");
    }

    SECTION("Hierarchy") {
        RemoveComponentCommand command{edit.session, edit.getRegistry(), {entity}, TechEngine::componentTypeId<TechEngine::Hierarchy>()};
        command.apply(edit.scene);

        CHECK(edit.scene.getParent(entity) == parent);
    }

    SECTION("a type the entity lacks") {
        RemoveComponentCommand command{edit.session, edit.getRegistry(), {entity}, sampleType()};
        command.apply(edit.scene);
    }

    CHECK(edit.session.getRecords().empty());
    CHECK(edit.snapshot().editRevision == before.editRevision);
}

template<typename Record>
static const Record* findRecord(const std::span<const EditRecord> records, const Entity entity) {
    for (const EditRecord& record: records) {
        const Record* typed = std::get_if<Record>(&record);
        if (typed != nullptr && typed->entity == entity) {
            return typed;
        }
    }
    return nullptr;
}

static void checkEuler(const EditTestScene& edit, const Entity entity, const Vec3& degrees) {
    const Vec3 actual = Transform::toEulerDegrees(edit.scene.getComponent<Transform>(entity).getLocal().rotation);
    CHECK(actual.x == Catch::Approx(degrees.x).margin(1e-3));
    CHECK(actual.y == Catch::Approx(degrees.y).margin(1e-3));
    CHECK(actual.z == Catch::Approx(degrees.z).margin(1e-3));
}

TEST_CASE("a set field over three entities changes that field on each and leaves three records", "[editor][edit][commands]") {
    EditTestScene edit;
    const std::vector<Entity> entities = {edit.createNamed("A"), edit.createNamed("B"), edit.createNamed("C")};
    for (std::size_t i = 0; i < entities.size(); i++) {
        const float offset = static_cast<float>(i + 1);
        REQUIRE(edit.scene.getComponent<Transform>(entities[i]).setLocal({Vec3(offset, offset * 10.0f, offset * 100.0f), Quat(1.0f, 0.0f, 0.0f, 0.0f), Vec3(1.0f)}));
    }
    const EditorSnapshot before = edit.snapshot();

    SetFieldCommand command{edit.session, edit.getRegistry(), entities, transformType(), TRANSFORM_POSITION_X_FIELD, FieldValue{5.0f}};
    command.apply(edit.scene);

    const std::span<const EditRecord> records = edit.session.getRecords();
    CHECK(records.size() == entities.size());
    for (std::size_t i = 0; i < entities.size(); i++) {
        const float offset = static_cast<float>(i + 1);
        CHECK(edit.scene.getComponent<Transform>(entities[i]).getLocal().position == Vec3(5.0f, offset * 10.0f, offset * 100.0f));
        const FieldEditRecord* record = findRecord<FieldEditRecord>(records, entities[i]);
        REQUIRE(record != nullptr);
        CHECK(record->component == transformType());
        CHECK(record->field == TRANSFORM_POSITION_X_FIELD);
        CHECK(record->oldValue == FieldValue{offset});
    }
    CHECK(edit.snapshot().editRevision > before.editRevision);
}

TEST_CASE("a set field over several skips an entity that holds the value, is dead or lacks the component, and edits the rest", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity edited = edit.createNamed("Edited");
    const Entity holds = edit.createNamed("Holds");
    const Entity lacks = edit.createNamed("Lacks");
    const Entity dead = edit.createNamed("Dead");
    edit.scene.addComponent<InspectedSample>(edited);
    edit.scene.addComponent<InspectedSample>(holds);
    edit.scene.addComponent<InspectedSample>(dead);
    edit.scene.getComponent<InspectedSample>(holds).count = 7;
    REQUIRE(edit.scene.destroyEntity(dead));
    const EditorSnapshot before = edit.snapshot();

    SetFieldCommand command{edit.session, edit.getRegistry(), {edited, holds, lacks, dead}, sampleType(), SAMPLE_COUNT_FIELD, FieldValue{std::int32_t{7}}};
    command.apply(edit.scene);

    CHECK(edit.scene.getComponent<InspectedSample>(edited).count == 7);
    CHECK(edit.scene.getComponent<InspectedSample>(holds).count == 7);
    CHECK_FALSE(edit.scene.hasComponent<InspectedSample>(lacks));
    REQUIRE(edit.session.getRecords().size() == 1);
    const FieldEditRecord* record = findRecord<FieldEditRecord>(edit.session.getRecords(), edited);
    REQUIRE(record != nullptr);
    CHECK(record->oldValue == FieldValue{std::int32_t{3}});
    CHECK(edit.snapshot().editRevision > before.editRevision);
}

TEST_CASE("an add over a mixed selection adds only where the type is missing", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity holds = edit.createNamed("Holds");
    const Entity first = edit.createNamed("First");
    const Entity second = edit.createNamed("Second");
    edit.scene.addComponent<InspectedSample>(holds);
    edit.scene.getComponent<InspectedSample>(holds).count = 11;
    const EditorSnapshot before = edit.snapshot();

    AddComponentCommand command{edit.session, {holds, first, second}, sampleType()};
    command.apply(edit.scene);

    CHECK(edit.scene.getComponent<InspectedSample>(holds).count == 11);
    REQUIRE(edit.scene.hasComponent<InspectedSample>(first));
    REQUIRE(edit.scene.hasComponent<InspectedSample>(second));
    CHECK(edit.scene.getComponent<InspectedSample>(first).count == InspectedSample{}.count);
    CHECK(edit.scene.getComponent<InspectedSample>(second).count == InspectedSample{}.count);

    const std::span<const EditRecord> records = edit.session.getRecords();
    CHECK(records.size() == 2);
    CHECK(findRecord<ComponentAddedRecord>(records, holds) == nullptr);
    const ComponentAddedRecord* addedFirst = findRecord<ComponentAddedRecord>(records, first);
    const ComponentAddedRecord* addedSecond = findRecord<ComponentAddedRecord>(records, second);
    REQUIRE(addedFirst != nullptr);
    REQUIRE(addedSecond != nullptr);
    CHECK(addedFirst->component == sampleType());
    CHECK(addedSecond->component == sampleType());
    CHECK(edit.snapshot().editRevision > before.editRevision);
}

TEST_CASE("a remove over several removes from all and records each one's payload", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity first = edit.createNamed("First");
    const Entity second = edit.createNamed("Second");
    edit.scene.addComponent<InspectedSample>(first);
    edit.scene.addComponent<InspectedSample>(second);
    edit.scene.getComponent<InspectedSample>(first).label = "First";
    edit.scene.getComponent<InspectedSample>(second).label = "Second";
    const TechEngine::ComponentTypeRecord* typeRecord = edit.getRegistry().find(sampleType());
    REQUIRE(typeRecord != nullptr);
    std::vector<std::byte> firstPayload;
    std::vector<std::byte> secondPayload;
    TechEngine::Writer firstWriter{firstPayload};
    TechEngine::Writer secondWriter{secondPayload};
    typeRecord->writePayload(firstWriter, &edit.scene.getComponent<InspectedSample>(first));
    typeRecord->writePayload(secondWriter, &edit.scene.getComponent<InspectedSample>(second));
    REQUIRE(firstPayload != secondPayload);
    const EditorSnapshot before = edit.snapshot();

    RemoveComponentCommand command{edit.session, edit.getRegistry(), {first, second}, sampleType()};
    command.apply(edit.scene);

    CHECK_FALSE(edit.scene.hasComponent<InspectedSample>(first));
    CHECK_FALSE(edit.scene.hasComponent<InspectedSample>(second));
    const std::span<const EditRecord> records = edit.session.getRecords();
    CHECK(records.size() == 2);
    const ComponentRemovedRecord* removedFirst = findRecord<ComponentRemovedRecord>(records, first);
    const ComponentRemovedRecord* removedSecond = findRecord<ComponentRemovedRecord>(records, second);
    REQUIRE(removedFirst != nullptr);
    REQUIRE(removedSecond != nullptr);
    CHECK(removedFirst->payload == firstPayload);
    CHECK(removedSecond->payload == secondPayload);
    CHECK(edit.snapshot().editRevision > before.editRevision);
}

TEST_CASE("a rotation axis edit keeps each entity's other two axes", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity active = edit.createNamed("Active");
    const Entity other = edit.createNamed("Other");
    const Entity third = edit.createNamed("Third");
    setEuler(edit, active, Vec3(10.0f, 20.0f, 30.0f));
    setEuler(edit, other, Vec3(40.0f, 50.0f, 60.0f));
    setEuler(edit, third, Vec3(-70.0f, -10.0f, 120.0f));
    const std::array<Entity, 3> entities = {active, other, third};
    std::array<Quat, 3> oldRotations{};
    for (std::size_t i = 0; i < entities.size(); i++) {
        oldRotations[i] = edit.scene.getComponent<Transform>(entities[i]).getLocal().rotation;
    }
    const EditorSnapshot before = edit.snapshot();

    std::uint32_t axis = 0;
    float degrees = 0.0f;
    std::array<Vec3, 3> expected{};
    SECTION("X") {
        axis = 0;
        degrees = 0.0f;
        expected = {Vec3(0.0f, 20.0f, 30.0f), Vec3(0.0f, 50.0f, 60.0f), Vec3(0.0f, -10.0f, 120.0f)};
    }
    SECTION("Y") {
        axis = 1;
        degrees = 15.0f;
        expected = {Vec3(10.0f, 15.0f, 30.0f), Vec3(40.0f, 15.0f, 60.0f), Vec3(-70.0f, 15.0f, 120.0f)};
    }
    SECTION("Z") {
        axis = 2;
        degrees = -45.0f;
        expected = {Vec3(10.0f, 20.0f, -45.0f), Vec3(40.0f, 50.0f, -45.0f), Vec3(-70.0f, -10.0f, -45.0f)};
    }
    const Quat activeRotation = Transform::fromEulerDegrees(expected[0]);

    SetRotationAxisCommand command{edit.session, edit.getRegistry(), {active, other, third}, active, activeRotation, axis, degrees};
    command.apply(edit.scene);

    CHECK(edit.scene.getComponent<Transform>(active).getLocal().rotation == activeRotation);
    checkEuler(edit, other, expected[1]);
    checkEuler(edit, third, expected[2]);
    const std::span<const EditRecord> records = edit.session.getRecords();
    CHECK(records.size() == entities.size());
    for (std::size_t i = 0; i < entities.size(); i++) {
        const RotationEditRecord* record = findRecord<RotationEditRecord>(records, entities[i]);
        REQUIRE(record != nullptr);
        CHECK(record->oldRotation == oldRotations[i]);
    }
    CHECK(edit.snapshot().editRevision > before.editRevision);
}

TEST_CASE("a rotation axis edit that leaves the rotation as it was records nothing and keeps the edit revision", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("A");
    setEuler(edit, entity, Vec3(10.0f, 20.0f, 30.0f));
    const Quat current = edit.scene.getComponent<Transform>(entity).getLocal().rotation;
    const EditorSnapshot before = edit.snapshot();

    SetRotationAxisCommand command{edit.session, edit.getRegistry(), {entity}, entity, current, 0, 10.0f};
    command.apply(edit.scene);

    CHECK(edit.scene.getComponent<Transform>(entity).getLocal().rotation == current);
    CHECK(edit.session.getRecords().empty());
    CHECK(edit.snapshot().editRevision == before.editRevision);
}

TEST_CASE("a rotation axis edit skips a dead entity and still edits the rest", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity active = edit.createNamed("Active");
    const Entity dead = edit.createNamed("Dead");
    REQUIRE(edit.scene.destroyEntity(dead));
    const Quat activeRotation = Transform::fromEulerDegrees(Vec3(0.0f, 30.0f, 0.0f));

    SetRotationAxisCommand command{edit.session, edit.getRegistry(), {active, dead}, active, activeRotation, 1, 30.0f};
    command.apply(edit.scene);

    CHECK(edit.scene.getComponent<Transform>(active).getLocal().rotation == activeRotation);
    REQUIRE(edit.session.getRecords().size() == 1);
    CHECK(findRecord<RotationEditRecord>(edit.session.getRecords(), active) != nullptr);
}

TEST_CASE("a rotation axis edit rotates an entity whose scale has a zero axis", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity active = edit.createNamed("Active");
    const Entity other = edit.createNamed("Other");
    for (const Entity entity: {active, other}) {
        applySetField(edit, entity, transformType(), TRANSFORM_SCALE_X_FIELD, FieldValue{0.0f});
        REQUIRE(edit.scene.getComponent<Transform>(entity).getLocal().scale.x == 0.0f);
    }
    const EditorSnapshot before = edit.snapshot();
    const Quat activeRotation = Transform::fromEulerDegrees(Vec3(0.0f, 30.0f, 0.0f));

    SetRotationAxisCommand command{edit.session, edit.getRegistry(), {active, other}, active, activeRotation, 1, 30.0f};
    command.apply(edit.scene);

    CHECK(edit.scene.getComponent<Transform>(active).getLocal().rotation == activeRotation);
    checkEuler(edit, other, Vec3(0.0f, 30.0f, 0.0f));
    CHECK(edit.scene.getComponent<Transform>(active).getLocal().scale.x == 0.0f);
    CHECK(edit.scene.getComponent<Transform>(other).getLocal().scale.x == 0.0f);
    CHECK(findRecord<RotationEditRecord>(edit.session.getRecords(), active) != nullptr);
    CHECK(findRecord<RotationEditRecord>(edit.session.getRecords(), other) != nullptr);
    CHECK(edit.snapshot().editRevision > before.editRevision);
}

TEST_CASE("a remove over several skips an entity it cannot remove from and removes from the rest", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity lacks = edit.createNamed("Lacks");
    const Entity holds = edit.createNamed("Holds");
    edit.scene.addComponent<InspectedSample>(holds);
    const EditorSnapshot before = edit.snapshot();

    RemoveComponentCommand command{edit.session, edit.getRegistry(), {lacks, holds}, sampleType()};
    command.apply(edit.scene);

    CHECK_FALSE(edit.scene.hasComponent<InspectedSample>(holds));
    REQUIRE(edit.session.getRecords().size() == 1);
    CHECK(findRecord<ComponentRemovedRecord>(edit.session.getRecords(), holds) != nullptr);
    CHECK(findRecord<ComponentRemovedRecord>(edit.session.getRecords(), lacks) == nullptr);
    CHECK(edit.snapshot().editRevision > before.editRevision);
}

static void setLocal(EditTestScene& edit, const Entity entity, const Vec3& position, const Vec3& degrees) {
    REQUIRE(edit.scene.getComponent<Transform>(entity).setLocal({position, Transform::fromEulerDegrees(degrees), Vec3(1.0f)}));
}

static void checkVec3(const Vec3& actual, const Vec3& expected) {
    CHECK(actual.x == Catch::Approx(expected.x).margin(1e-4));
    CHECK(actual.y == Catch::Approx(expected.y).margin(1e-4));
    CHECK(actual.z == Catch::Approx(expected.z).margin(1e-4));
}

TEST_CASE("a world position edit on a child of a moved parent sets the local value that puts it there", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity parent = edit.createNamed("Parent");
    const Entity child = edit.createNamed("Child", parent);
    setLocal(edit, child, Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f));
    const EditorSnapshot before = edit.snapshot();

    std::uint32_t axis = 0;
    float value = 0.0f;
    Vec3 expectedLocal(0.0f);
    SECTION("a parent that is only moved") {
        setLocal(edit, parent, Vec3(10.0f, 0.0f, 0.0f), Vec3(0.0f));
        axis = 0;
        value = 12.0f;
        expectedLocal = Vec3(2.0f, 0.0f, 0.0f);
    }
    SECTION("a parent that is moved and turned 90 degrees about Z") {
        setLocal(edit, parent, Vec3(10.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 90.0f));
        // The child sits at (10, 1, 0) in the world; moving it to y = 3 is two more along the parent's local X.
        axis = 1;
        value = 3.0f;
        expectedLocal = Vec3(3.0f, 0.0f, 0.0f);
    }
    const TransformValues oldLocal = edit.scene.getComponent<Transform>(child).getLocal();

    SetWorldAxisCommand command{edit.session, {child}, child, Quat(1.0f, 0.0f, 0.0f, 0.0f), TransformPart::Position, axis, value};
    command.apply(edit.scene);

    const Transform& transform = edit.scene.getComponent<Transform>(child);
    checkVec3(transform.getLocal().position, expectedLocal);
    CHECK(transform.getWorld().position[static_cast<int>(axis)] == Catch::Approx(value).margin(1e-4));
    REQUIRE(edit.session.getRecords().size() == 1);
    const LocalTransformRecord* record = findRecord<LocalTransformRecord>(edit.session.getRecords(), child);
    REQUIRE(record != nullptr);
    CHECK(record->oldLocal.position == oldLocal.position);
    CHECK(record->oldLocal.rotation == oldLocal.rotation);
    CHECK(record->oldLocal.scale == oldLocal.scale);
    CHECK(edit.snapshot().editRevision > before.editRevision);
}

TEST_CASE("a world edit over several keeps each entity's other world axes", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity firstParent = edit.createNamed("First parent");
    const Entity secondParent = edit.createNamed("Second parent");
    const Entity first = edit.createNamed("First", firstParent);
    const Entity second = edit.createNamed("Second", secondParent);
    setLocal(edit, firstParent, Vec3(1.0f, 2.0f, 3.0f), Vec3(0.0f));
    setLocal(edit, secondParent, Vec3(-4.0f, 0.0f, 5.0f), Vec3(0.0f, 90.0f, 0.0f));
    setLocal(edit, second, Vec3(1.0f, 1.0f, 0.0f), Vec3(0.0f));
    const Vec3 firstWorld = edit.scene.getComponent<Transform>(first).getWorld().position;
    const Vec3 secondWorld = edit.scene.getComponent<Transform>(second).getWorld().position;

    SetWorldAxisCommand command{edit.session, {first, second}, first, Quat(1.0f, 0.0f, 0.0f, 0.0f), TransformPart::Position, 1, 7.0f};
    command.apply(edit.scene);

    checkVec3(edit.scene.getComponent<Transform>(first).getWorld().position, Vec3(firstWorld.x, 7.0f, firstWorld.z));
    checkVec3(edit.scene.getComponent<Transform>(second).getWorld().position, Vec3(secondWorld.x, 7.0f, secondWorld.z));
    CHECK(edit.session.getRecords().size() == 2);
    CHECK(findRecord<LocalTransformRecord>(edit.session.getRecords(), first) != nullptr);
    CHECK(findRecord<LocalTransformRecord>(edit.session.getRecords(), second) != nullptr);
}

TEST_CASE("a world rotation edit gives the active entity the sent world rotation and each other entity its own other two axes", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity parent = edit.createNamed("Parent");
    const Entity active = edit.createNamed("Active", parent);
    const Entity other = edit.createNamed("Other");
    setLocal(edit, parent, Vec3(0.0f), Vec3(0.0f, 0.0f, 30.0f));
    setLocal(edit, other, Vec3(0.0f), Vec3(10.0f, 20.0f, 30.0f));
    const Quat activeRotation = Transform::fromEulerDegrees(Vec3(0.0f, 45.0f, 30.0f));

    SetWorldAxisCommand command{edit.session, {active, other}, active, activeRotation, TransformPart::Rotation, 1, 45.0f};
    command.apply(edit.scene);

    const Quat activeWorld = edit.scene.getComponent<Transform>(active).getWorld().rotation;
    CHECK(std::abs(glm::dot(activeWorld, activeRotation)) == Catch::Approx(1.0f).margin(1e-5));
    checkEuler(edit, other, Vec3(10.0f, 45.0f, 30.0f));
    CHECK(edit.session.getRecords().size() == 2);
}

TEST_CASE("a world edit that leaves the world transform as it was records nothing and keeps the edit revision", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity parent = edit.createNamed("Parent");
    const Entity child = edit.createNamed("Child", parent);
    setLocal(edit, parent, Vec3(10.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 90.0f));
    setLocal(edit, child, Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f));
    const float currentX = edit.scene.getComponent<Transform>(child).getWorld().position.x;
    const TransformValues oldLocal = edit.scene.getComponent<Transform>(child).getLocal();
    const EditorSnapshot before = edit.snapshot();

    SetWorldAxisCommand command{edit.session, {child}, child, Quat(1.0f, 0.0f, 0.0f, 0.0f), TransformPart::Position, 0, currentX};
    command.apply(edit.scene);

    const TransformValues& local = edit.scene.getComponent<Transform>(child).getLocal();
    CHECK(local.position == oldLocal.position);
    CHECK(local.rotation == oldLocal.rotation);
    CHECK(local.scale == oldLocal.scale);
    CHECK(edit.session.getRecords().empty());
    CHECK(edit.snapshot().editRevision == before.editRevision);
}

TEST_CASE("a world edit skips a dead entity and one it cannot convert, and still edits the rest", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity live = edit.createNamed("Live");
    const Entity dead = edit.createNamed("Dead");
    REQUIRE(edit.scene.destroyEntity(dead));
    const Entity flatParent = edit.createNamed("Flat parent");
    const Entity flatChild = edit.createNamed("Flat child", flatParent);
    applySetField(edit, flatParent, transformType(), TRANSFORM_SCALE_X_FIELD, FieldValue{0.0f});
    REQUIRE(edit.scene.getComponent<Transform>(flatParent).getLocal().scale.x == 0.0f);
    const std::size_t recordsBefore = edit.session.getRecords().size();

    SetWorldAxisCommand command{edit.session, {live, dead, flatChild}, live, Quat(1.0f, 0.0f, 0.0f, 0.0f), TransformPart::Position, 2, 4.0f};
    command.apply(edit.scene);

    CHECK(edit.scene.getComponent<Transform>(live).getLocal().position.z == Catch::Approx(4.0f));
    CHECK(edit.session.getRecords().size() == recordsBefore + 1);
    CHECK(findRecord<LocalTransformRecord>(edit.session.getRecords(), live) != nullptr);
    CHECK(findRecord<LocalTransformRecord>(edit.session.getRecords(), flatChild) == nullptr);
}

static ComponentTypeId meshRendererType() {
    return TechEngine::componentTypeId<MeshRenderer>();
}

static void applyAssetSet(AssetEditHarness& edit, std::vector<Entity> entities, const ResourceId id) {
    SetAssetFieldCommand command{edit.session, edit.getRegistry(), edit.cache, edit.sceneHandle, edit.index, std::move(entities), meshRendererType(), MESH_RENDERER_MESH_FIELD, id};
    command.apply(edit.scene);
    // App::applyCommands runs a loading phase after every batch, so the tests do too.
    edit.cache.runLoadingPhase();
}

static void checkReadyMesh(AssetEditHarness& edit, const Entity entity, const ResourceId id) {
    const MeshRenderer& renderer = edit.scene.getComponent<MeshRenderer>(entity);
    CHECK(renderer.mesh.id == id);
    REQUIRE(renderer.mesh.handle.valid());
    ResourceState state = ResourceState::Pending;
    REQUIRE(edit.cache.getState(renderer.mesh.handle, state));
    CHECK(state == ResourceState::Ready);
    const TechEngine::Mesh* mesh = edit.cache.resolve(renderer.mesh.handle);
    REQUIRE(mesh != nullptr);
    CHECK(mesh->id == id);
}

// A fresh request counts one acquire only when no dependency of the scene already holds the entry.
static void checkNotHeld(AssetEditHarness& edit, const ResourceId id) {
    TechEngine::ResourceHandle<TechEngine::Mesh> handle;
    REQUIRE(edit.cache.request(id, handle) == std::error_code{});
    std::uint32_t acquires = 0;
    REQUIRE(edit.cache.getAcquireCount(handle, acquires));
    CHECK(acquires == 1);
    edit.cache.release(handle);
}

static void checkEmptyMesh(const AssetEditHarness& edit, const Entity entity) {
    const MeshRenderer& renderer = edit.scene.getComponent<MeshRenderer>(entity);
    CHECK(renderer.mesh.id == ResourceId{});
    CHECK_FALSE(renderer.mesh.handle.valid());
}

TEST_CASE("an asset set over a known mesh fills the handle", "[editor][edit][commands][assets]") {
    AssetEditHarness edit;
    const Entity entity = edit.createWithMeshRenderer("Crate");
    const std::uint64_t revisionBefore = edit.snapshot().editRevision;

    applyAssetSet(edit, {entity}, EDIT_MESH_ID);

    checkReadyMesh(edit, entity, EDIT_MESH_ID);
    REQUIRE(edit.session.getRecords().size() == 1);
    const FieldEditRecord* record = findRecord<FieldEditRecord>(edit.session.getRecords(), entity);
    REQUIRE(record != nullptr);
    CHECK(record->component == meshRendererType());
    CHECK(record->field == MESH_RENDERER_MESH_FIELD);
    CHECK(record->oldValue == FieldValue{ResourceId{}});
    CHECK(edit.snapshot().editRevision > revisionBefore);
}

TEST_CASE("an asset set over an unknown id is refused, and changes and records nothing", "[editor][edit][commands][assets]") {
    AssetEditHarness edit;
    const Entity entity = edit.createWithMeshRenderer("Crate");
    const std::uint64_t revisionBefore = edit.snapshot().editRevision;

    applyAssetSet(edit, {entity}, EDIT_UNKNOWN_ID);

    checkEmptyMesh(edit, entity);
    CHECK(edit.session.getRecords().empty());
    CHECK(edit.snapshot().editRevision == revisionBefore);
}

TEST_CASE("an asset set over an id of another asset type than the field's is refused", "[editor][edit][commands][assets]") {
    AssetEditHarness edit;
    const Entity entity = edit.createWithMeshRenderer("Crate");
    const std::uint64_t revisionBefore = edit.snapshot().editRevision;

    applyAssetSet(edit, {entity}, EDIT_OTHER_SCENE_ID);

    checkEmptyMesh(edit, entity);
    CHECK(edit.session.getRecords().empty());
    CHECK(edit.snapshot().editRevision == revisionBefore);
}

TEST_CASE("an asset set over a server asset is refused, and the scene gains no dependency", "[editor][edit][commands][assets]") {
    AssetEditHarness edit;
    const Entity entity = edit.createWithMeshRenderer("Crate");
    const std::uint64_t revisionBefore = edit.snapshot().editRevision;

    applyAssetSet(edit, {entity}, EDIT_SERVER_MESH_ID);

    checkEmptyMesh(edit, entity);
    CHECK(edit.session.getRecords().empty());
    CHECK(edit.snapshot().editRevision == revisionBefore);
    checkNotHeld(edit, EDIT_SERVER_MESH_ID);
}

TEST_CASE("an asset set over a component type the registry does not know is refused", "[editor][edit][commands][assets]") {
    AssetEditHarness edit;
    const Entity entity = edit.createWithMeshRenderer("Crate");
    const ComponentTypeId unknown{TechEngine::StringId("Tests.Unregistered")};
    const std::uint64_t revisionBefore = edit.snapshot().editRevision;

    SetAssetFieldCommand command{edit.session, edit.getRegistry(), edit.cache, edit.sceneHandle, edit.index, {entity}, unknown, MESH_RENDERER_MESH_FIELD, EDIT_MESH_ID};
    command.apply(edit.scene);

    checkEmptyMesh(edit, entity);
    CHECK(edit.session.getRecords().empty());
    CHECK(edit.snapshot().editRevision == revisionBefore);
    checkNotHeld(edit, EDIT_MESH_ID);
}

TEST_CASE("an asset set on a field that is not an asset field is refused, and the scene gains no dependency", "[editor][edit][commands][assets]") {
    AssetEditHarness edit;
    const Entity entity = edit.createWithMeshRenderer("Crate");
    const float positionBefore = edit.scene.getComponent<Transform>(entity).getLocal().position.x;

    SetAssetFieldCommand command{edit.session, edit.getRegistry(), edit.cache, edit.sceneHandle, edit.index, {entity}, transformType(), TRANSFORM_POSITION_X_FIELD, EDIT_MESH_ID};
    command.apply(edit.scene);

    CHECK(edit.scene.getComponent<Transform>(entity).getLocal().position.x == positionBefore);
    CHECK(edit.session.getRecords().empty());
    checkNotHeld(edit, EDIT_MESH_ID);
}

TEST_CASE("an asset set over several skips an invalid, a dead and a bare entity and one that holds the id, and edits the rest", "[editor][edit][commands][assets]") {
    AssetEditHarness edit;
    const Entity holder = edit.createWithMeshRenderer("Holder");
    applyAssetSet(edit, {holder}, EDIT_MESH_ID);
    const Entity edited = edit.createWithMeshRenderer("Edited");
    const Entity dead = edit.createWithMeshRenderer("Dead");
    REQUIRE(edit.scene.destroyEntity(dead));
    const Entity bare = edit.scene.createEntity();
    REQUIRE(bare.valid());
    const std::size_t recordsBefore = edit.session.getRecords().size();

    applyAssetSet(edit, {Entity{}, dead, bare, holder, edited}, EDIT_MESH_ID);

    checkReadyMesh(edit, holder, EDIT_MESH_ID);
    checkReadyMesh(edit, edited, EDIT_MESH_ID);
    CHECK_FALSE(edit.scene.hasComponent<MeshRenderer>(bare));
    CHECK(edit.session.getRecords().size() == recordsBefore + 1);
    CHECK(findRecord<FieldEditRecord>(edit.session.getRecords().subspan(recordsBefore), edited) != nullptr);
}

TEST_CASE("an asset set whose mesh fails to load leaves the field as it was and records nothing", "[editor][edit][commands][assets]") {
    AssetEditHarness edit;
    const Entity entity = edit.createWithMeshRenderer("Crate");
    applyAssetSet(edit, {entity}, EDIT_MESH_ID);
    REQUIRE(edit.session.getRecords().size() == 1);
    const std::uint64_t revisionBefore = edit.snapshot().editRevision;

    applyAssetSet(edit, {entity}, EDIT_BROKEN_MESH_ID);

    checkReadyMesh(edit, entity, EDIT_MESH_ID);
    CHECK(edit.session.getRecords().size() == 1);
    CHECK(edit.snapshot().editRevision == revisionBefore);
}

TEST_CASE("an asset set over a mesh that already failed to load is refused too", "[editor][edit][commands][assets]") {
    AssetEditHarness edit;
    const Entity first = edit.createWithMeshRenderer("First");
    const Entity second = edit.createWithMeshRenderer("Second");
    applyAssetSet(edit, {first}, EDIT_BROKEN_MESH_ID);
    const std::uint64_t revisionBefore = edit.snapshot().editRevision;

    applyAssetSet(edit, {second}, EDIT_BROKEN_MESH_ID);

    checkEmptyMesh(edit, first);
    checkEmptyMesh(edit, second);
    CHECK(edit.session.getRecords().empty());
    CHECK(edit.snapshot().editRevision == revisionBefore);
}

TEST_CASE("an asset set on a scene with no cache entry is refused", "[editor][edit][commands][assets]") {
    AssetEditHarness edit;
    const Entity entity = edit.createWithMeshRenderer("Crate");
    edit.releaseSceneEntry();
    const std::uint64_t revisionBefore = edit.snapshot().editRevision;

    applyAssetSet(edit, {entity}, EDIT_MESH_ID);

    checkEmptyMesh(edit, entity);
    CHECK(edit.session.getRecords().empty());
    CHECK(edit.snapshot().editRevision == revisionBefore);
}

TEST_CASE("an asset set over three entities fills each handle with the same entry and leaves three records", "[editor][edit][commands][assets]") {
    AssetEditHarness edit;
    const std::vector<Entity> entities = {edit.createWithMeshRenderer("A"), edit.createWithMeshRenderer("B"), edit.createWithMeshRenderer("C")};

    applyAssetSet(edit, entities, EDIT_MESH_ID);

    for (const Entity entity: entities) {
        checkReadyMesh(edit, entity, EDIT_MESH_ID);
        CHECK(findRecord<FieldEditRecord>(edit.session.getRecords(), entity) != nullptr);
    }
    CHECK(edit.session.getRecords().size() == 3);
    const TechEngine::ResourceHandle<TechEngine::Mesh> handle = edit.scene.getComponent<MeshRenderer>(entities[0]).mesh.handle;
    CHECK(edit.scene.getComponent<MeshRenderer>(entities[1]).mesh.handle == handle);
    CHECK(edit.scene.getComponent<MeshRenderer>(entities[2]).mesh.handle == handle);
}

TEST_CASE("an asset set to another mesh replaces the id and the handle, and records the old id", "[editor][edit][commands][assets]") {
    AssetEditHarness edit;
    const Entity entity = edit.createWithMeshRenderer("Crate");
    applyAssetSet(edit, {entity}, EDIT_MESH_ID);

    applyAssetSet(edit, {entity}, EDIT_SECOND_MESH_ID);

    checkReadyMesh(edit, entity, EDIT_SECOND_MESH_ID);
    const std::span<const EditRecord> records = edit.session.getRecords();
    REQUIRE(records.size() == 2);
    const FieldEditRecord* last = std::get_if<FieldEditRecord>(&records[1]);
    REQUIRE(last != nullptr);
    CHECK(last->oldValue == FieldValue{EDIT_MESH_ID});
}

TEST_CASE("an asset set to the id the field already holds keeps the edit revision and records nothing", "[editor][edit][commands][assets]") {
    AssetEditHarness edit;
    const Entity entity = edit.createWithMeshRenderer("Crate");
    applyAssetSet(edit, {entity}, EDIT_MESH_ID);
    const std::uint64_t revisionBefore = edit.snapshot().editRevision;

    applyAssetSet(edit, {entity}, EDIT_MESH_ID);

    checkReadyMesh(edit, entity, EDIT_MESH_ID);
    CHECK(edit.session.getRecords().size() == 1);
    CHECK(edit.snapshot().editRevision == revisionBefore);
}

TEST_CASE("an asset row holds the field's id and the asset type the picker filters on", "[editor][edit][assets]") {
    AssetEditHarness edit;
    const Entity entity = edit.createWithMeshRenderer("Crate");
    edit.scene.getComponent<MeshRenderer>(entity).mesh.id = EDIT_MESH_ID;
    edit.selectOnly(entity);

    const EditorSnapshot snapshot = edit.snapshot();

    const ComponentRow& row = componentRowFor(snapshot, meshRendererType());
    REQUIRE(row.fields.size() > MESH_RENDERER_MESH_FIELD);
    const TechEngine::FieldRow& field = row.fields[MESH_RENDERER_MESH_FIELD];
    CHECK(field.kind == TechEngine::FieldKind::Asset);
    CHECK(field.value == FieldValue{EDIT_MESH_ID});
    CHECK(field.assetType == TechEngine::MESH_TYPE_TAG);
}
