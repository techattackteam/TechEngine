#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <scene/SceneTestRegistry.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

static_assert(!TechEngineTests::WritableSceneComponent<TechEngine::Hierarchy>);
static_assert(!TechEngineTests::AddableSceneComponent<TechEngine::Hierarchy>);
static_assert(!TechEngineTests::RemovableSceneComponent<TechEngine::Hierarchy>);

struct HierarchyTestMarker {
    std::uint32_t value = 0;
};

static void registerHierarchyTestComponents(TechEngine::ComponentRegistry& registry) {
    TechEngineTests::registerBuiltInSceneComponents(registry);
    registry.registerComponent<HierarchyTestMarker>("Tests.HierarchyTestMarker");
}

TEST_CASE("hierarchy is publicly registerable and readable through Scene", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity entity = scene.createEntity();
    const TechEngine::Scene& constScene = scene;

    const TechEngine::Hierarchy& hierarchy = scene.getComponent<TechEngine::Hierarchy>(entity);

    REQUIRE(scene.hasComponent<TechEngine::Hierarchy>(entity));
    REQUIRE(&hierarchy == &constScene.getComponent<TechEngine::Hierarchy>(entity));
}

TEST_CASE("hierarchy keeps roots and ordered children", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity root = scene.createEntity();
    const TechEngine::Entity first = scene.createEntity();
    const TechEngine::Entity second = scene.createEntity();
    const TechEngine::Entity third = scene.createEntity();

    REQUIRE(scene.setParent(first, root, 0));
    REQUIRE(scene.setParent(second, root, 1));
    REQUIRE(scene.setParent(third, root, 1));

    const std::vector<TechEngine::Entity> expectedChildren{first, third, second};
    REQUIRE(scene.getChildren(root) == expectedChildren);
    REQUIRE(scene.getParent(first) == root);
    REQUIRE(scene.getParent(third) == root);

    const std::vector<TechEngine::Entity> expectedRoots{root};
    REQUIRE(scene.getRoots() == expectedRoots);

    REQUIRE(scene.reorderChild(second, 0));
    const std::vector<TechEngine::Entity> reorderedChildren{second, first, third};
    REQUIRE(scene.getChildren(root) == reorderedChildren);

    REQUIRE(scene.unparent(first));
    const std::vector<TechEngine::Entity> remainingChildren{second, third};
    REQUIRE(scene.getChildren(root) == remainingChildren);
    REQUIRE_FALSE(scene.getParent(first).valid());

    REQUIRE_FALSE(scene.reorderChild(third, 2));
    REQUIRE(scene.getChildren(root) == remainingChildren);
    REQUIRE(scene.getParent(third) == root);
}

TEST_CASE("hierarchy rejects cycles without changing links", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity root = scene.createEntity();
    const TechEngine::Entity middle = scene.createEntity();
    const TechEngine::Entity leaf = scene.createEntity();

    REQUIRE(scene.setParent(middle, root, 0));
    REQUIRE(scene.setParent(leaf, middle, 0));
    REQUIRE_FALSE(scene.setParent(root, leaf, 0));
    REQUIRE_FALSE(scene.setParent(middle, middle, 0));

    REQUIRE_FALSE(scene.getParent(root).valid());
    REQUIRE(scene.getParent(middle) == root);
    REQUIRE(scene.getParent(leaf) == middle);
    const std::vector<TechEngine::Entity> rootChildren{middle};
    const std::vector<TechEngine::Entity> middleChildren{leaf};
    REQUIRE(scene.getChildren(root) == rootChildren);
    REQUIRE(scene.getChildren(middle) == middleChildren);
}

TEST_CASE("reparenting and insertion validate position before changing links", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity left = scene.createEntity();
    const TechEngine::Entity right = scene.createEntity();
    const TechEngine::Entity first = scene.createEntity();
    const TechEngine::Entity second = scene.createEntity();
    const TechEngine::Entity third = scene.createEntity();

    REQUIRE(scene.setParent(first, left, 0));
    REQUIRE(scene.setParent(second, left, 1));
    REQUIRE(scene.setParent(third, left, 2));
    REQUIRE(scene.setParent(second, right, 0));
    REQUIRE(scene.setParent(third, left, 0));

    const std::vector<TechEngine::Entity> leftChildren{third, first};
    const std::vector<TechEngine::Entity> rightChildren{second};
    REQUIRE(scene.getChildren(left) == leftChildren);
    REQUIRE(scene.getChildren(right) == rightChildren);
    REQUIRE(scene.getParent(second) == right);

    REQUIRE_FALSE(scene.setParent(third, right, 2));
    REQUIRE_FALSE(scene.setParent(first, left, 2));
    REQUIRE(scene.getChildren(left) == leftChildren);
    REQUIRE(scene.getChildren(right) == rightChildren);
    REQUIRE(scene.getParent(third) == left);

    const TechEngine::Entity stale = scene.createEntity();
    REQUIRE(scene.destroyEntity(stale));
    REQUIRE_FALSE(scene.setParent(third, stale, 0));
    REQUIRE(scene.getChildren(left) == leftChildren);
}

TEST_CASE("destroying a subtree preserves its surviving siblings", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity root = scene.createEntity();
    const TechEngine::Entity branch = scene.createEntity();
    const TechEngine::Entity sibling = scene.createEntity();
    const TechEngine::Entity leaf = scene.createEntity();

    REQUIRE(scene.setParent(branch, root, 0));
    REQUIRE(scene.setParent(sibling, root, 1));
    REQUIRE(scene.setParent(leaf, branch, 0));
    REQUIRE(scene.destroyEntity(branch));

    REQUIRE_FALSE(scene.contains(branch));
    REQUIRE_FALSE(scene.contains(leaf));
    REQUIRE(scene.contains(root));
    REQUIRE(scene.contains(sibling));
    const std::vector<TechEngine::Entity> expectedChildren{sibling};
    REQUIRE(scene.getChildren(root) == expectedChildren);
    REQUIRE(scene.getParent(sibling) == root);
}

TEST_CASE("detached and reparented subtrees survive their former parent", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity root = scene.createEntity();
    const TechEngine::Entity newRoot = scene.createEntity();
    const TechEngine::Entity detached = scene.createEntity();
    const TechEngine::Entity detachedLeaf = scene.createEntity();
    const TechEngine::Entity moved = scene.createEntity();
    const TechEngine::Entity movedLeaf = scene.createEntity();

    REQUIRE(scene.setParent(detached, root, 0));
    REQUIRE(scene.setParent(moved, root, 1));
    REQUIRE(scene.setParent(detachedLeaf, detached, 0));
    REQUIRE(scene.setParent(movedLeaf, moved, 0));
    REQUIRE(scene.unparent(detached));
    REQUIRE(scene.setParent(moved, newRoot, 0));
    REQUIRE(scene.destroyEntity(root));

    REQUIRE(scene.contains(detached));
    REQUIRE(scene.contains(detachedLeaf));
    REQUIRE(scene.contains(moved));
    REQUIRE(scene.contains(movedLeaf));
    REQUIRE_FALSE(scene.getParent(detached).valid());
    REQUIRE(scene.getParent(detachedLeaf) == detached);
    REQUIRE(scene.getParent(moved) == newRoot);
    REQUIRE(scene.getParent(movedLeaf) == moved);
    REQUIRE(scene.getChildren(newRoot) == std::vector<TechEngine::Entity>{moved});
}

TEST_CASE("reused entity slots do not inherit old hierarchy links", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity root = scene.createEntity();
    const TechEngine::Entity oldChild = scene.createEntity();

    REQUIRE(scene.setParent(oldChild, root, 0));
    REQUIRE(scene.destroyEntity(oldChild));
    const TechEngine::Entity replacement = scene.createEntity();

    REQUIRE(replacement.index == oldChild.index);
    REQUIRE(replacement.generation != oldChild.generation);
    REQUIRE_FALSE(scene.contains(oldChild));
    REQUIRE_FALSE(scene.getParent(replacement).valid());
    REQUIRE(scene.getChildren(root).empty());
    REQUIRE_FALSE(scene.setParent(oldChild, root, 0));
}

TEST_CASE("clearing a scene invalidates hierarchy links and entity handles", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity root = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();

    REQUIRE(scene.setParent(child, root, 0));
    scene.clear();

    REQUIRE_FALSE(scene.contains(root));
    REQUIRE_FALSE(scene.contains(child));
    REQUIRE(scene.getRoots().empty());
    REQUIRE(scene.getChildren(root).empty());
    REQUIRE_FALSE(scene.getParent(child).valid());
    REQUIRE_FALSE(scene.destroyEntity(root));
    REQUIRE_FALSE(scene.unparent(child));
    REQUIRE_FALSE(scene.reorderChild(child, 0));

    const TechEngine::Entity replacement = scene.createEntity();
    REQUIRE(scene.contains(replacement));
    REQUIRE_FALSE(scene.contains(root));
    REQUIRE_FALSE(scene.contains(child));
    REQUIRE_FALSE(scene.getParent(replacement).valid());
    REQUIRE(scene.getChildren(replacement).empty());
    REQUIRE(scene.getRoots() == std::vector<TechEngine::Entity>{replacement});
    REQUIRE_FALSE(scene.unparent(replacement));
    REQUIRE(scene.reorderChild(replacement, 0));
    REQUIRE_FALSE(scene.reorderChild(replacement, 1));

    const TechEngine::Entity second = scene.createEntity();
    const std::vector<TechEngine::Entity> expectedRoots{replacement, second};
    REQUIRE(scene.getRoots() == expectedRoots);
}

TEST_CASE("deep subtree destruction does not depend on recursion depth", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    std::vector<TechEngine::Entity> chain;
    chain.reserve(4096);

    chain.push_back(scene.createEntity());
    for (int i = 1; i < 4096; i++) {
        const TechEngine::Entity child = scene.createEntity();
        REQUIRE(scene.setParent(child, chain.back(), 0));
        chain.push_back(child);
    }

    REQUIRE(scene.destroyEntity(chain.front()));
    for (const TechEngine::Entity entity: chain) {
        REQUIRE_FALSE(scene.contains(entity));
    }
}

TEST_CASE("appending a child lands last after every change to the previous last child", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity root = scene.createEntity();
    const TechEngine::Entity first = scene.createEntity();
    const TechEngine::Entity second = scene.createEntity();
    const TechEngine::Entity third = scene.createEntity();
    const TechEngine::Entity appended = scene.createEntity();
    REQUIRE(scene.setParent(first, root, 0));
    REQUIRE(scene.setParent(second, root, 1));
    REQUIRE(scene.setParent(third, root, 2));

    SECTION("the last child is unparented") {
        REQUIRE(scene.unparent(third));
        REQUIRE(scene.setParent(appended, root, 2));

        const std::vector<TechEngine::Entity> expected{first, second, appended};
        REQUIRE(scene.getChildren(root) == expected);
    }

    SECTION("the last child is destroyed") {
        REQUIRE(scene.destroyEntity(third));
        REQUIRE(scene.setParent(appended, root, 2));

        const std::vector<TechEngine::Entity> expected{first, second, appended};
        REQUIRE(scene.getChildren(root) == expected);
    }

    SECTION("the last child moves to another parent") {
        const TechEngine::Entity other = scene.createEntity();
        REQUIRE(scene.setParent(third, other, 0));
        REQUIRE(scene.setParent(appended, root, 2));
        const TechEngine::Entity otherAppended = scene.createEntity();
        REQUIRE(scene.setParent(otherAppended, other, 1));

        const std::vector<TechEngine::Entity> expectedRoot{first, second, appended};
        const std::vector<TechEngine::Entity> expectedOther{third, otherAppended};
        REQUIRE(scene.getChildren(root) == expectedRoot);
        REQUIRE(scene.getChildren(other) == expectedOther);
    }

    SECTION("the last child is reordered to the front") {
        REQUIRE(scene.reorderChild(third, 0));
        REQUIRE(scene.setParent(appended, root, 3));

        const std::vector<TechEngine::Entity> expected{third, first, second, appended};
        REQUIRE(scene.getChildren(root) == expected);
    }

    SECTION("a middle child is reordered to the end") {
        REQUIRE(scene.reorderChild(first, 2));
        REQUIRE(scene.setParent(appended, root, 3));

        const std::vector<TechEngine::Entity> expected{second, third, first, appended};
        REQUIRE(scene.getChildren(root) == expected);
    }

    SECTION("the last child is reordered onto its own position") {
        REQUIRE(scene.reorderChild(third, 2));
        REQUIRE(scene.setParent(appended, root, 3));

        const std::vector<TechEngine::Entity> expected{first, second, third, appended};
        REQUIRE(scene.getChildren(root) == expected);
    }

    SECTION("every child is removed") {
        REQUIRE(scene.unparent(third));
        REQUIRE(scene.unparent(first));
        REQUIRE(scene.unparent(second));
        REQUIRE(scene.setParent(appended, root, 0));

        const std::vector<TechEngine::Entity> expected{appended};
        REQUIRE(scene.getChildren(root) == expected);
    }

    REQUIRE(scene.getParent(appended) == root);
}

TEST_CASE("a wide parent keeps the order its children were appended in", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity root = scene.createEntity();

    std::vector<TechEngine::Entity> expected;
    for (std::size_t i = 0; i < 4096; i++) {
        const TechEngine::Entity child = scene.createEntity();
        REQUIRE(scene.setParent(child, root, i));
        expected.push_back(child);
    }

    REQUIRE(scene.getChildren(root) == expected);
}

TEST_CASE("a root that gains or loses a component keeps its position", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registerHierarchyTestComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity first = scene.createEntity();
    const TechEngine::Entity second = scene.createEntity();
    const TechEngine::Entity third = scene.createEntity();
    const std::vector<TechEngine::Entity> expected{first, second, third};

    scene.addComponent<HierarchyTestMarker>(first);
    REQUIRE(scene.getRoots() == expected);

    scene.addComponent<HierarchyTestMarker>(second);
    scene.removeComponent<HierarchyTestMarker>(first);
    REQUIRE(scene.getRoots() == expected);

    REQUIRE(scene.addComponentRaw(third, TechEngine::componentTypeId<HierarchyTestMarker>()));
    REQUIRE(scene.removeComponentRaw(second, TechEngine::componentTypeId<HierarchyTestMarker>()));
    REQUIRE(scene.getRoots() == expected);
}

TEST_CASE("destroying a root leaves the other roots in order", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity first = scene.createEntity();
    const TechEngine::Entity second = scene.createEntity();
    const TechEngine::Entity third = scene.createEntity();
    const TechEngine::Entity fourth = scene.createEntity();
    const TechEngine::Entity secondChild = scene.createEntity();
    REQUIRE(scene.setParent(secondChild, second, 0));

    SECTION("a middle root with a child") {
        REQUIRE(scene.destroyEntity(second));

        REQUIRE_FALSE(scene.contains(secondChild));
        const std::vector<TechEngine::Entity> expected{first, third, fourth};
        REQUIRE(scene.getRoots() == expected);
    }

    SECTION("the first root") {
        REQUIRE(scene.destroyEntity(first));

        const std::vector<TechEngine::Entity> expected{second, third, fourth};
        REQUIRE(scene.getRoots() == expected);
    }

    SECTION("the last root, and a new entity takes its place at the end") {
        REQUIRE(scene.destroyEntity(fourth));
        const TechEngine::Entity appended = scene.createEntity();

        const std::vector<TechEngine::Entity> expected{first, second, third, appended};
        REQUIRE(scene.getRoots() == expected);
    }

    SECTION("a reused slot joins the roots at the end, not at its old position") {
        REQUIRE(scene.destroyEntity(first));
        const TechEngine::Entity replacement = scene.createEntity();

        REQUIRE(replacement.index == first.index);
        const std::vector<TechEngine::Entity> expected{second, third, fourth, replacement};
        REQUIRE(scene.getRoots() == expected);
    }

    SECTION("every root") {
        REQUIRE(scene.destroyEntity(third));
        REQUIRE(scene.destroyEntity(first));
        REQUIRE(scene.destroyEntity(fourth));
        REQUIRE(scene.destroyEntity(second));
        REQUIRE(scene.getRoots().empty());

        const TechEngine::Entity only = scene.createEntity();
        REQUIRE(scene.getRoots() == std::vector<TechEngine::Entity>{only});
    }
}

TEST_CASE("a root that becomes a child leaves the roots, and unparenting appends it at the end", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity first = scene.createEntity();
    const TechEngine::Entity second = scene.createEntity();
    const TechEngine::Entity third = scene.createEntity();

    REQUIRE(scene.setParent(second, first, 0));
    const std::vector<TechEngine::Entity> withoutSecond{first, third};
    REQUIRE(scene.getRoots() == withoutSecond);

    REQUIRE(scene.unparent(second));
    const std::vector<TechEngine::Entity> secondLast{first, third, second};
    REQUIRE(scene.getRoots() == secondLast);

    REQUIRE(scene.setParent(third, second, 0, TechEngine::ReparentMode::PreserveWorld));
    REQUIRE(scene.unparent(third, TechEngine::ReparentMode::PreserveWorld));
    const std::vector<TechEngine::Entity> thirdLast{first, second, third};
    REQUIRE(scene.getRoots() == thirdLast);
}

TEST_CASE("reorderChild moves a root among the roots and refuses a position past the end", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity first = scene.createEntity();
    const TechEngine::Entity second = scene.createEntity();
    const TechEngine::Entity third = scene.createEntity();

    REQUIRE(scene.reorderChild(third, 0));
    const std::vector<TechEngine::Entity> thirdFirst{third, first, second};
    REQUIRE(scene.getRoots() == thirdFirst);

    REQUIRE(scene.reorderChild(third, 2));
    const std::vector<TechEngine::Entity> original{first, second, third};
    REQUIRE(scene.getRoots() == original);

    REQUIRE(scene.reorderChild(first, 1));
    const std::vector<TechEngine::Entity> firstMiddle{second, first, third};
    REQUIRE(scene.getRoots() == firstMiddle);

    REQUIRE_FALSE(scene.reorderChild(first, 3));
    REQUIRE(scene.getRoots() == firstMiddle);

    REQUIRE(scene.reorderChild(third, 0));
    const TechEngine::Entity appended = scene.createEntity();
    const std::vector<TechEngine::Entity> afterAppend{third, second, first, appended};
    REQUIRE(scene.getRoots() == afterAppend);
}

TEST_CASE("unparent at a position inserts the child there among the roots", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    TechEngine::Scene scene(registry);
    const TechEngine::Entity parent = scene.createEntity();
    const TechEngine::Entity other = scene.createEntity();
    const TechEngine::Entity child = scene.createEntity();
    REQUIRE(scene.setParent(child, parent, 0));

    SECTION("keeping its local transform") {
        REQUIRE(scene.unparent(child, 1, TechEngine::ReparentMode::PreserveLocal));

        REQUIRE_FALSE(scene.getParent(child).valid());
        REQUIRE(scene.getChildren(parent).empty());
        const std::vector<TechEngine::Entity> expected{parent, child, other};
        REQUIRE(scene.getRoots() == expected);
    }

    SECTION("keeping its world transform") {
        TechEngine::TransformValues parentLocal;
        parentLocal.position = TechEngine::Vec3(5.0f, 0.0f, 0.0f);
        REQUIRE(scene.getComponent<TechEngine::Transform>(parent).setLocal(parentLocal));
        TechEngine::TransformValues childLocal;
        childLocal.position = TechEngine::Vec3(1.0f, 2.0f, 0.0f);
        REQUIRE(scene.getComponent<TechEngine::Transform>(child).setLocal(childLocal));

        REQUIRE(scene.unparent(child, 0, TechEngine::ReparentMode::PreserveWorld));

        const std::vector<TechEngine::Entity> expected{child, parent, other};
        REQUIRE(scene.getRoots() == expected);
        const TechEngine::Vec3 position = scene.getComponent<TechEngine::Transform>(child).getLocal().position;
        REQUIRE(position.x == Catch::Approx(6.0f).margin(1e-5));
        REQUIRE(position.y == Catch::Approx(2.0f).margin(1e-5));
        REQUIRE(position.z == Catch::Approx(0.0f).margin(1e-5));
    }

    SECTION("at the end") {
        REQUIRE(scene.unparent(child, 2, TechEngine::ReparentMode::PreserveLocal));

        const std::vector<TechEngine::Entity> expected{parent, other, child};
        REQUIRE(scene.getRoots() == expected);
    }

    SECTION("a position past the end changes nothing") {
        REQUIRE_FALSE(scene.unparent(child, 3, TechEngine::ReparentMode::PreserveLocal));
        REQUIRE_FALSE(scene.unparent(child, 3, TechEngine::ReparentMode::PreserveWorld));

        REQUIRE(scene.getParent(child) == parent);
        REQUIRE(scene.getChildren(parent) == std::vector<TechEngine::Entity>{child});
        const std::vector<TechEngine::Entity> expected{parent, other};
        REQUIRE(scene.getRoots() == expected);
    }

    SECTION("a root is refused, since it has no parent to leave") {
        REQUIRE_FALSE(scene.unparent(other, 0, TechEngine::ReparentMode::PreserveLocal));

        const std::vector<TechEngine::Entity> expected{parent, other};
        REQUIRE(scene.getRoots() == expected);
    }
}

TEST_CASE("many roots keep their order through component changes, destruction and moves", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registerHierarchyTestComponents(registry);
    TechEngine::Scene scene(registry);

    std::vector<TechEngine::Entity> expected;
    for (std::size_t i = 0; i < 512; i++) {
        expected.push_back(scene.createEntity());
    }

    for (std::size_t i = 0; i < expected.size(); i += 3) {
        scene.addComponent<HierarchyTestMarker>(expected[i]);
    }
    REQUIRE(scene.getRoots() == expected);

    std::vector<TechEngine::Entity> survivors;
    for (std::size_t i = 0; i < expected.size(); i++) {
        if (i % 5 == 0) {
            REQUIRE(scene.destroyEntity(expected[i]));
        } else {
            survivors.push_back(expected[i]);
        }
    }
    REQUIRE(scene.getRoots() == survivors);

    std::size_t removed = 0;
    for (std::size_t i = 0; i < survivors.size(); i += 2) {
        if (scene.hasComponent<HierarchyTestMarker>(survivors[i])) {
            scene.removeComponent<HierarchyTestMarker>(survivors[i]);
            removed++;
        }
    }
    REQUIRE(removed > 0);
    REQUIRE(scene.getRoots() == survivors);

    const TechEngine::Entity holder = survivors.front();
    std::vector<TechEngine::Entity> roots;
    std::vector<TechEngine::Entity> children;
    for (std::size_t i = 1; i < survivors.size(); i++) {
        if (i % 2 == 0) {
            REQUIRE(scene.setParent(survivors[i], holder, children.size()));
            children.push_back(survivors[i]);
        } else {
            roots.push_back(survivors[i]);
        }
    }
    roots.insert(roots.begin(), holder);
    REQUIRE(scene.getRoots() == roots);
    REQUIRE(scene.getChildren(holder) == children);
}
