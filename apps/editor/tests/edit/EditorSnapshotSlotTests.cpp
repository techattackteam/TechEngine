#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <edit/EditorSnapshot.hpp>
#include <edit/EditorSnapshotSlot.hpp>
#include <edit/FieldValue.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

using TechEngine::EditorSnapshot;
using TechEngine::EditorSnapshotSlot;
using TechEngine::Entity;
using TechEngine::HierarchyRow;

static EditorSnapshot snapshotAt(const std::uint64_t editRevision) {
    EditorSnapshot snapshot;
    snapshot.editRevision = editRevision;
    return snapshot;
}

TEST_CASE("take finds nothing before the first publish and leaves its target alone", "[editor][edit]") {
    EditorSnapshotSlot slot;
    EditorSnapshot out = snapshotAt(7);

    CHECK_FALSE(slot.take(out));
    CHECK(out.editRevision == 7);
}

TEST_CASE("take returns the newest of two publishes, then nothing until the next publish", "[editor][edit]") {
    EditorSnapshotSlot slot;
    EditorSnapshot out;

    slot.publish(snapshotAt(1));
    slot.publish(snapshotAt(2));

    REQUIRE(slot.take(out));
    CHECK(out.editRevision == 2);
    CHECK_FALSE(slot.take(out));
    CHECK(out.editRevision == 2);

    slot.publish(snapshotAt(3));

    REQUIRE(slot.take(out));
    CHECK(out.editRevision == 3);
}

TEST_CASE("a taken snapshot carries every row, the components, the selection and both revisions", "[editor][edit]") {
    EditorSnapshotSlot slot;
    EditorSnapshot published;
    HierarchyRow parent;
    parent.entity = Entity{0, 0};
    parent.name = "Parent";
    parent.childCount = 1;
    HierarchyRow child;
    child.entity = Entity{1, 0};
    child.depth = 1;
    child.name = "Child";
    published.hierarchy = {parent, child};
    published.selection = {parent.entity, child.entity};
    published.active = child.entity;
    published.selectionSequence = 3;
    TechEngine::FieldRow field;
    field.index = 2;
    field.name = "position.z";
    field.kind = TechEngine::FieldKind::Float;
    field.value = 3.0f;
    TechEngine::ComponentRow component;
    component.type = TechEngine::ComponentTypeId{TechEngine::StringId("Tests.Inspected")};
    component.tag = "Tests.Inspected";
    component.fields = {field};
    published.components = {component};
    published.addable = {TechEngine::ComponentChoice{TechEngine::ComponentTypeId{TechEngine::StringId("Tests.Addable")}, "Tests.Addable"}};
    published.editRevision = 5;
    published.savedRevision = 4;
    slot.publish(published);

    EditorSnapshot out;
    REQUIRE(slot.take(out));

    REQUIRE(out.components.size() == 1);
    CHECK(out.components[0].type == component.type);
    CHECK(out.components[0].tag == "Tests.Inspected");
    REQUIRE(out.components[0].fields.size() == 1);
    CHECK(out.components[0].fields[0].index == 2);
    CHECK(out.components[0].fields[0].name == "position.z");
    CHECK(out.components[0].fields[0].kind == TechEngine::FieldKind::Float);
    CHECK(out.components[0].fields[0].value == TechEngine::FieldValue{3.0f});
    REQUIRE(out.addable.size() == 1);
    CHECK(out.addable[0].tag == "Tests.Addable");

    REQUIRE(out.hierarchy.size() == 2);
    CHECK(out.hierarchy[0].entity == parent.entity);
    CHECK(out.hierarchy[0].name == "Parent");
    CHECK(out.hierarchy[0].childCount == 1);
    CHECK(out.hierarchy[1].entity == child.entity);
    CHECK(out.hierarchy[1].depth == 1);
    CHECK(out.hierarchy[1].name == "Child");
    CHECK(out.selection == std::vector<Entity>{parent.entity, child.entity});
    CHECK(out.active == child.entity);
    CHECK(out.selectionSequence == 3);
    CHECK(out.editRevision == 5);
    CHECK(out.savedRevision == 4);
}

TEST_CASE("main never takes an older snapshot than one it already took while the simulation publishes", "[editor][edit][threads]") {
    constexpr std::uint64_t LAST_REVISION = 2000;
    EditorSnapshotSlot slot;
    std::atomic<bool> published = false;
    std::jthread simulation{[&] {
        for (std::uint64_t revision = 1; revision <= LAST_REVISION; revision++) {
            slot.publish(snapshotAt(revision));
        }
        published = true;
    }};

    std::uint64_t newest = 0;
    bool ordered = true;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
    while (newest < LAST_REVISION && std::chrono::steady_clock::now() < deadline) {
        EditorSnapshot out;
        if (slot.take(out)) {
            ordered = ordered && out.editRevision > newest;
            newest = out.editRevision;
        }
    }
    simulation.join();

    CHECK(published.load());
    CHECK(ordered);
    CHECK(newest == LAST_REVISION);
    EditorSnapshot after;
    CHECK_FALSE(slot.take(after));
}
