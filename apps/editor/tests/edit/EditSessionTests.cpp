#include <TechEngine/core/scene/Entity.hpp>

#include <edit/EditSession.hpp>
#include <edit/EditTestScene.hpp>
#include <edit/EditorSnapshot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

using TechEngine::EditorSnapshot;
using TechEngine::Entity;
using TechEngine::HierarchyRow;
using TechEngineTests::EditTestScene;
using TechEngineTests::rowFor;
using TechEngineTests::rowIndex;

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

TEST_CASE("building into a snapshot that already holds rows replaces them", "[editor][edit]") {
    EditTestScene edit;
    const Entity a = edit.createNamed("A");
    edit.createNamed("B", a);
    EditorSnapshot snapshot;
    edit.session.buildSnapshot(edit.scene, snapshot);

    edit.session.buildSnapshot(edit.scene, snapshot);

    REQUIRE(snapshot.hierarchy.size() == 2);
    CHECK(rowFor(snapshot, a).childCount == 1);
}
