#include <TechEngine/core/scene/Entity.hpp>

#include <edit/EditorSnapshot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

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

static constexpr Entity PARENT{10, 0};
static constexpr Entity FIRST{11, 0};
static constexpr Entity SECOND{12, 0};
static constexpr Entity THIRD{13, 0};

static EditorSnapshot threeSiblings() {
    EditorSnapshot snapshot;
    snapshot.hierarchy = {row(PARENT, 0, 3), row(FIRST, 1, 0), row(SECOND, 1, 0), row(THIRD, 1, 0)};
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

TEST_CASE("findRow gives a listed entity's row and refuses an unlisted or stale one", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();
    std::size_t index = 99;

    REQUIRE(snapshot.findRow(SECOND_CHILD, index));
    CHECK(index == 3);
    REQUIRE(snapshot.findRow(ROOT, index));
    CHECK(index == 0);

    index = 99;
    CHECK_FALSE(snapshot.findRow(Entity{9, 0}, index));
    CHECK_FALSE(snapshot.findRow(Entity{0, 1}, index));
    CHECK_FALSE(snapshot.findRow(Entity{}, index));
    CHECK(index == 99);
}

TEST_CASE("a row's parent is the nearest row above it one level up", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();

    CHECK_FALSE(snapshot.getParentOfRow(0).valid());
    CHECK(snapshot.getParentOfRow(1) == ROOT);
    CHECK(snapshot.getParentOfRow(2) == CHILD);
    CHECK(snapshot.getParentOfRow(3) == ROOT);
    CHECK_FALSE(snapshot.getParentOfRow(4).valid());
}

TEST_CASE("a row's sibling index skips the deeper rows between siblings", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();

    CHECK(snapshot.getSiblingIndexOfRow(1) == 0);
    CHECK(snapshot.getSiblingIndexOfRow(2) == 0);
    CHECK(snapshot.getSiblingIndexOfRow(3) == 1);
    CHECK(snapshot.getSiblingIndexOfRow(0) == 0);
}

TEST_CASE("a drop under a new parent keeps the gap as the position", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();
    std::size_t position = 99;

    REQUIRE(snapshot.resolveDrop(GRANDCHILD, OTHER_ROOT, 0, position));
    CHECK(position == 0);
    REQUIRE(snapshot.resolveDrop(GRANDCHILD, ROOT, 1, position));
    CHECK(position == 1);
}

TEST_CASE("a drop inside the same parent counts the position among the other children", "[editor][edit]") {
    const EditorSnapshot snapshot = threeSiblings();
    std::size_t position = 99;

    SECTION("moving down lands one gap earlier") {
        REQUIRE(snapshot.resolveDrop(FIRST, PARENT, 2, position));
        CHECK(position == 1);
        REQUIRE(snapshot.resolveDrop(FIRST, PARENT, 3, position));
        CHECK(position == 2);
    }

    SECTION("moving up keeps the gap") {
        REQUIRE(snapshot.resolveDrop(THIRD, PARENT, 0, position));
        CHECK(position == 0);
        REQUIRE(snapshot.resolveDrop(THIRD, PARENT, 1, position));
        CHECK(position == 1);
    }

    SECTION("the gaps just above and below the dragged entity change nothing") {
        CHECK_FALSE(snapshot.resolveDrop(SECOND, PARENT, 1, position));
        CHECK_FALSE(snapshot.resolveDrop(SECOND, PARENT, 2, position));
        CHECK_FALSE(snapshot.resolveDrop(FIRST, PARENT, 0, position));
    }
}

TEST_CASE("a drop into the dragged entity's own subtree is refused", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();
    std::size_t position = 99;

    CHECK_FALSE(snapshot.resolveDrop(ROOT, ROOT, 0, position));
    CHECK_FALSE(snapshot.resolveDrop(ROOT, CHILD, 0, position));
    CHECK_FALSE(snapshot.resolveDrop(ROOT, GRANDCHILD, 0, position));
    CHECK(position == 99);
}

TEST_CASE("only a child can be dropped at the root, because roots have no order", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();
    std::size_t position = 99;

    CHECK(snapshot.resolveDrop(CHILD, Entity{}, 0, position));
    CHECK_FALSE(snapshot.resolveDrop(OTHER_ROOT, Entity{}, 0, position));
    CHECK_FALSE(snapshot.resolveDrop(ROOT, Entity{}, 1, position));
}

TEST_CASE("a drop of an entity the snapshot does not list is refused", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();
    std::size_t position = 99;

    CHECK_FALSE(snapshot.resolveDrop(Entity{9, 0}, ROOT, 0, position));
    CHECK(position == 99);
}

TEST_CASE("the topmost selected entities leave out every selected descendant, in hierarchy order", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();
    std::vector<Entity> topmost = {Entity{9, 0}};

    SECTION("an ancestor and its descendants, given out of order") {
        snapshot.collectTopmost(std::vector<Entity>{GRANDCHILD, OTHER_ROOT, CHILD, ROOT}, topmost);

        CHECK(topmost == std::vector<Entity>{ROOT, OTHER_ROOT});
    }

    SECTION("siblings and a cousin with no selected ancestor") {
        snapshot.collectTopmost(std::vector<Entity>{SECOND_CHILD, GRANDCHILD}, topmost);

        CHECK(topmost == std::vector<Entity>{GRANDCHILD, SECOND_CHILD});
    }

    SECTION("a selected grandchild whose parent is not selected but whose grandparent is") {
        snapshot.collectTopmost(std::vector<Entity>{GRANDCHILD, ROOT}, topmost);

        CHECK(topmost == std::vector<Entity>{ROOT});
    }

    SECTION("an entity the snapshot does not list") {
        snapshot.collectTopmost(std::vector<Entity>{Entity{9, 0}, CHILD, Entity{1, 1}}, topmost);

        CHECK(topmost == std::vector<Entity>{CHILD});
    }

    SECTION("an empty selection") {
        snapshot.collectTopmost(std::vector<Entity>{}, topmost);

        CHECK(topmost.empty());
    }
}

TEST_CASE("a drop of several counts the position among the parent's children that are not dragged", "[editor][edit]") {
    const EditorSnapshot snapshot = threeSiblings();
    std::size_t position = 99;

    SECTION("the first and the last to the end") {
        REQUIRE(snapshot.resolveDrop(std::vector<Entity>{FIRST, THIRD}, PARENT, 3, position));
        CHECK(position == 1);
    }

    SECTION("the first and the last between the first and the second") {
        REQUIRE(snapshot.resolveDrop(std::vector<Entity>{FIRST, THIRD}, PARENT, 1, position));
        CHECK(position == 0);
    }

    SECTION("the second and the last to the top") {
        REQUIRE(snapshot.resolveDrop(std::vector<Entity>{SECOND, THIRD}, PARENT, 0, position));
        CHECK(position == 0);
    }
}

TEST_CASE("a drop of several that leaves them all where they are is refused", "[editor][edit]") {
    const EditorSnapshot snapshot = threeSiblings();
    std::size_t position = 99;

    CHECK_FALSE(snapshot.resolveDrop(std::vector<Entity>{FIRST, SECOND}, PARENT, 0, position));
    CHECK_FALSE(snapshot.resolveDrop(std::vector<Entity>{FIRST, SECOND}, PARENT, 1, position));
    CHECK_FALSE(snapshot.resolveDrop(std::vector<Entity>{FIRST, SECOND}, PARENT, 2, position));
}

TEST_CASE("a drop of several under a new parent keeps the gap as the position", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();
    std::size_t position = 99;

    REQUIRE(snapshot.resolveDrop(std::vector<Entity>{GRANDCHILD, OTHER_ROOT}, SECOND_CHILD, 0, position));
    CHECK(position == 0);
}

TEST_CASE("a drop of several into any dragged subtree is refused", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();
    std::size_t position = 99;

    CHECK_FALSE(snapshot.resolveDrop(std::vector<Entity>{CHILD, OTHER_ROOT}, GRANDCHILD, 0, position));
    CHECK_FALSE(snapshot.resolveDrop(std::vector<Entity>{CHILD, OTHER_ROOT}, CHILD, 0, position));
    CHECK_FALSE(snapshot.resolveDrop(std::vector<Entity>{SECOND_CHILD, OTHER_ROOT}, OTHER_ROOT, 0, position));
    CHECK(position == 99);
}

TEST_CASE("a drop of several with one the snapshot does not list is refused", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();
    std::size_t position = 99;

    CHECK_FALSE(snapshot.resolveDrop(std::vector<Entity>{GRANDCHILD, Entity{9, 0}}, OTHER_ROOT, 0, position));
    CHECK(position == 99);
}

TEST_CASE("a drop of several at the root is allowed while one of them is a child", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();
    std::size_t position = 99;

    SECTION("two children") {
        REQUIRE(snapshot.resolveDrop(std::vector<Entity>{GRANDCHILD, SECOND_CHILD}, Entity{}, 0, position));
        CHECK(position == 0);
    }

    SECTION("a child beside a root") {
        REQUIRE(snapshot.resolveDrop(std::vector<Entity>{SECOND_CHILD, OTHER_ROOT}, Entity{}, 0, position));
        CHECK(position == 0);
    }

    SECTION("only roots") {
        CHECK_FALSE(snapshot.resolveDrop(std::vector<Entity>{ROOT, OTHER_ROOT}, Entity{}, 0, position));
        CHECK(position == 99);
    }
}

TEST_CASE("a drop of several counts only the parent's direct children, skipping deeper rows", "[editor][edit]") {
    const EditorSnapshot snapshot = twoRoots();
    std::size_t position = 99;

    REQUIRE(snapshot.resolveDrop(std::vector<Entity>{SECOND_CHILD, OTHER_ROOT}, ROOT, 0, position));
    CHECK(position == 0);
    REQUIRE(snapshot.resolveDrop(std::vector<Entity>{GRANDCHILD, OTHER_ROOT}, ROOT, 2, position));
    CHECK(position == 2);
}
