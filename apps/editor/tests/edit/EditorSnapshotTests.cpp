#include <TechEngine/core/scene/Entity.hpp>

#include <edit/EditorSnapshot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>

using TechEngine::EditorSnapshot;
using TechEngine::Entity;
using TechEngine::HierarchyRow;

static constexpr Entity ROOT{0, 0};
static constexpr Entity CHILD{1, 0};
static constexpr Entity GRANDCHILD{2, 0};
static constexpr Entity SECOND_CHILD{3, 0};
static constexpr Entity OTHER_ROOT{4, 0};

static HierarchyRow row(const Entity entity, const std::uint32_t depth, const std::size_t childCount) {
    HierarchyRow result;
    result.entity = entity;
    result.depth = depth;
    result.name = "Entity";
    result.childCount = childCount;
    return result;
}

static EditorSnapshot twoRoots() {
    EditorSnapshot snapshot;
    snapshot.hierarchy = {row(ROOT, 0, 2), row(CHILD, 1, 1), row(GRANDCHILD, 2, 0), row(SECOND_CHILD, 1, 0), row(OTHER_ROOT, 0, 0)};
    return snapshot;
}

TEST_CASE("a drop onto the dragged entity itself is refused", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();

    CHECK(snapshot.isInSubtree(ROOT, ROOT));
    CHECK(snapshot.isInSubtree(GRANDCHILD, GRANDCHILD));
    CHECK(snapshot.isInSubtree(OTHER_ROOT, OTHER_ROOT));
}

TEST_CASE("a drop onto any descendant of the dragged entity is refused", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();

    CHECK(snapshot.isInSubtree(CHILD, ROOT));
    CHECK(snapshot.isInSubtree(GRANDCHILD, ROOT));
    CHECK(snapshot.isInSubtree(SECOND_CHILD, ROOT));
    CHECK(snapshot.isInSubtree(GRANDCHILD, CHILD));
}

TEST_CASE("a drop onto a parent, a sibling, a cousin or another root is allowed", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();

    CHECK_FALSE(snapshot.isInSubtree(ROOT, CHILD));
    CHECK_FALSE(snapshot.isInSubtree(SECOND_CHILD, CHILD));
    CHECK_FALSE(snapshot.isInSubtree(CHILD, SECOND_CHILD));
    CHECK_FALSE(snapshot.isInSubtree(GRANDCHILD, SECOND_CHILD));
    CHECK_FALSE(snapshot.isInSubtree(OTHER_ROOT, ROOT));
    CHECK_FALSE(snapshot.isInSubtree(ROOT, OTHER_ROOT));
}

TEST_CASE("a subtree ends at the next row that is not deeper, even at the last row", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();

    CHECK_FALSE(snapshot.isInSubtree(SECOND_CHILD, GRANDCHILD));
    CHECK_FALSE(snapshot.isInSubtree(OTHER_ROOT, SECOND_CHILD));
    CHECK_FALSE(snapshot.isInSubtree(ROOT, GRANDCHILD));
}

TEST_CASE("an entity the snapshot does not list is in no subtree", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();
    constexpr Entity unlisted{9, 0};
    constexpr Entity staleRoot{0, 1};

    CHECK_FALSE(snapshot.isInSubtree(unlisted, ROOT));
    CHECK_FALSE(snapshot.isInSubtree(ROOT, unlisted));
    CHECK_FALSE(snapshot.isInSubtree(staleRoot, ROOT));
    CHECK_FALSE(snapshot.isInSubtree(CHILD, staleRoot));
    CHECK_FALSE(snapshot.isInSubtree(Entity{}, Entity{}));
}

TEST_CASE("an empty snapshot has no subtrees", "[editor][edit]") {
    const EditorSnapshot snapshot{};

    CHECK_FALSE(snapshot.isInSubtree(ROOT, ROOT));
}
