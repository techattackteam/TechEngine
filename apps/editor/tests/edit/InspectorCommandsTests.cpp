#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Name.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <edit/EditRecord.hpp>
#include <edit/EditTestScene.hpp>
#include <edit/EditorSnapshot.hpp>
#include <edit/FieldValue.hpp>
#include <edit/InspectorCommands.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using TechEngine::AddComponentCommand;
using TechEngine::ComponentAddedRecord;
using TechEngine::ComponentRemovedRecord;
using TechEngine::ComponentTypeId;
using TechEngine::EditorSnapshot;
using TechEngine::EditRecord;
using TechEngine::Entity;
using TechEngine::FieldEditRecord;
using TechEngine::FieldValue;
using TechEngine::RemoveComponentCommand;
using TechEngine::SetFieldCommand;
using TechEngine::Transform;
using TechEngine::TransformValues;
using TechEngine::Vec3;
using TechEngineTests::componentRowFor;
using TechEngineTests::EditTestScene;
using TechEngineTests::findComponentRow;
using TechEngineTests::InspectedSample;
using TechEngineTests::SAMPLE_ASSET_FIELD;
using TechEngineTests::SAMPLE_COUNT_FIELD;
using TechEngineTests::SAMPLE_FIELD_COUNT;
using TechEngineTests::SAMPLE_GROUP_FIELD;
using TechEngineTests::SAMPLE_INDICES_FIELD;
using TechEngineTests::SAMPLE_LABEL_FIELD;
using TechEngineTests::SAMPLE_TARGET_FIELD;
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
    SetFieldCommand command{edit.session, edit.getRegistry(), entity, component, field, std::move(value)};
    command.apply(edit.scene);
}

TEST_CASE("set field changes exactly one field, publishes the new row and records the old value", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("A");
    REQUIRE(edit.scene.getComponent<Transform>(entity).setLocal({Vec3(1.0f, 2.0f, 3.0f), TechEngine::Quat(1.0f, 0.0f, 0.0f, 0.0f), Vec3(1.0f)}));
    edit.session.setSelected(entity);
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

TEST_CASE("set field to the value the field already holds keeps the edit revision", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("A");
    const EditorSnapshot before = edit.snapshot();

    applySetField(edit, entity, transformType(), TRANSFORM_POSITION_Y_FIELD, FieldValue{0.0f});

    CHECK(edit.snapshot().editRevision == before.editRevision);
}

TEST_CASE("set field writes every editable kind, a string included", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("A");
    edit.scene.addComponent<InspectedSample>(entity);

    applySetField(edit, entity, sampleType(), SAMPLE_COUNT_FIELD, FieldValue{std::int32_t{-9}});
    applySetField(edit, entity, sampleType(), SAMPLE_LABEL_FIELD, FieldValue{std::string("Changed")});

    const InspectedSample& sample = edit.scene.getComponent<InspectedSample>(entity);
    CHECK(sample.count == -9);
    CHECK(sample.label == "Changed");
    REQUIRE(edit.session.getRecords().size() == 2);
    CHECK(std::get<FieldEditRecord>(edit.session.getRecords()[0]).oldValue == FieldValue{std::int32_t{3}});
    CHECK(std::get<FieldEditRecord>(edit.session.getRecords()[1]).oldValue == FieldValue{std::string("Sample")});
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
    edit.session.setSelected(entity);
    const EditorSnapshot before = edit.snapshot();

    AddComponentCommand command{edit.session, entity, sampleType()};
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
        AddComponentCommand command{edit.session, entity, transformType()};
        command.apply(edit.scene);
    }

    SECTION("Name") {
        AddComponentCommand command{edit.session, entity, TechEngine::componentTypeId<TechEngine::Name>()};
        command.apply(edit.scene);
    }

    SECTION("a type the entity already holds") {
        AddComponentCommand command{edit.session, entity, sampleType()};
        command.apply(edit.scene);

        CHECK(edit.scene.getComponent<InspectedSample>(entity).count == 11);
    }

    SECTION("a destroyed entity") {
        const Entity dead = edit.createNamed("Dead");
        REQUIRE(edit.scene.destroyEntity(dead));

        AddComponentCommand command{edit.session, dead, sampleType()};
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
    edit.session.setSelected(entity);
    const TechEngine::ComponentTypeRecord* typeRecord = edit.getRegistry().find(sampleType());
    REQUIRE(typeRecord != nullptr);
    std::vector<std::byte> expectedPayload;
    TechEngine::Writer writer{expectedPayload};
    typeRecord->writePayload(writer, &edit.scene.getComponent<InspectedSample>(entity));
    const EditorSnapshot before = edit.snapshot();

    RemoveComponentCommand command{edit.session, edit.getRegistry(), entity, sampleType()};
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
        RemoveComponentCommand command{edit.session, edit.getRegistry(), entity, transformType()};
        command.apply(edit.scene);

        CHECK(edit.scene.hasComponent<Transform>(entity));
    }

    SECTION("Name") {
        RemoveComponentCommand command{edit.session, edit.getRegistry(), entity, TechEngine::componentTypeId<TechEngine::Name>()};
        command.apply(edit.scene);

        CHECK(edit.scene.getComponent<TechEngine::Name>(entity).value == "A");
    }

    SECTION("Hierarchy") {
        RemoveComponentCommand command{edit.session, edit.getRegistry(), entity, TechEngine::componentTypeId<TechEngine::Hierarchy>()};
        command.apply(edit.scene);

        CHECK(edit.scene.getParent(entity) == parent);
    }

    SECTION("a type the entity lacks") {
        RemoveComponentCommand command{edit.session, edit.getRegistry(), entity, sampleType()};
        command.apply(edit.scene);
    }

    CHECK(edit.session.getRecords().empty());
    CHECK(edit.snapshot().editRevision == before.editRevision);
}
