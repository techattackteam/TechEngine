#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>

#include <scene/SceneTestRegistry.hpp>

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <vector>

template<typename Component>
concept MutableSceneComponent = requires(TechEngine::Scene& scene, const TechEngine::Entity entity) {
    { scene.getComponent<Component>(entity) } -> std::same_as<Component&>;
};

template<typename Component>
concept AddableSceneComponent = requires(TechEngine::Scene& scene, const TechEngine::Entity entity) { scene.addComponent<Component>(entity); };

template<typename Component>
concept RemovableSceneComponent = requires(TechEngine::Scene& scene, const TechEngine::Entity entity) { scene.removeComponent<Component>(entity); };

static_assert(!MutableSceneComponent<TechEngine::Hierarchy>);
static_assert(!AddableSceneComponent<TechEngine::Hierarchy>);
static_assert(!RemovableSceneComponent<TechEngine::Hierarchy>);

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
    REQUIRE_FALSE(scene.reorderChild(replacement, 0));
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
