#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <edit/EditTestScene.hpp>
#include <edit/EditorSnapshot.hpp>
#include <edit/HierarchyCommands.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <vector>

using TechEngine::CreateEntityCommand;
using TechEngine::DestroyEntityCommand;
using TechEngine::EditorSnapshot;
using TechEngine::Entity;
using TechEngine::RenameEntityCommand;
using TechEngine::ReparentEntityCommand;
using TechEngine::SelectEntityCommand;
using TechEngine::Transform;
using TechEngine::TransformValues;
using TechEngine::Vec3;
using TechEngineTests::EditTestScene;
using TechEngineTests::rowFor;

TEST_CASE("create with no parent adds one root named Entity and raises the edit revision", "[editor][edit][commands]") {
    EditTestScene edit;
    const EditorSnapshot before = edit.snapshot();

    CreateEntityCommand create{edit.session, Entity{}};
    create.apply(edit.scene);

    const EditorSnapshot after = edit.snapshot();
    REQUIRE(after.hierarchy.size() == 1);
    CHECK(after.hierarchy[0].depth == 0);
    CHECK(after.hierarchy[0].name == "Entity");
    CHECK(after.hierarchy[0].childCount == 0);
    CHECK(edit.scene.getRoots().size() == 1);
    CHECK(after.editRevision > before.editRevision);
}

TEST_CASE("create under a parent adds one child of that parent", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity parent = edit.createNamed("Parent");
    const Entity sibling = edit.createNamed("Sibling", parent);
    const EditorSnapshot before = edit.snapshot();

    CreateEntityCommand create{edit.session, parent};
    create.apply(edit.scene);

    const std::vector<Entity> children = edit.scene.getChildren(parent);
    REQUIRE(children.size() == 2);
    const Entity created = children[0] == sibling ? children[1] : children[0];
    CHECK(created != sibling);
    CHECK(edit.scene.getRoots().size() == 1);
    const EditorSnapshot after = edit.snapshot();
    CHECK(rowFor(after, parent).childCount == 2);
    CHECK(rowFor(after, created).depth == 1);
    CHECK(rowFor(after, created).name == "Entity");
    CHECK(after.editRevision > before.editRevision);
}

TEST_CASE("create under a dead parent leaves no entity behind and keeps the edit revision", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity parent = edit.createNamed("Parent");
    REQUIRE(edit.scene.destroyEntity(parent));
    const EditorSnapshot before = edit.snapshot();

    CreateEntityCommand create{edit.session, parent};
    create.apply(edit.scene);

    const EditorSnapshot after = edit.snapshot();
    CHECK(after.hierarchy.empty());
    CHECK(edit.scene.getRoots().empty());
    CHECK(after.editRevision == before.editRevision);
}

TEST_CASE("destroy removes the entity with its subtree and raises the edit revision", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity a = edit.createNamed("A");
    const Entity b = edit.createNamed("B", a);
    const Entity c = edit.createNamed("C", b);
    const Entity d = edit.createNamed("D");
    const EditorSnapshot before = edit.snapshot();

    DestroyEntityCommand destroy{edit.session, b};
    destroy.apply(edit.scene);

    CHECK_FALSE(edit.scene.contains(b));
    CHECK_FALSE(edit.scene.contains(c));
    CHECK(edit.scene.contains(a));
    CHECK(edit.scene.contains(d));
    const EditorSnapshot after = edit.snapshot();
    REQUIRE(after.hierarchy.size() == 2);
    CHECK(rowFor(after, a).childCount == 0);
    CHECK(after.editRevision > before.editRevision);
}

TEST_CASE("destroy clears a selection inside the destroyed subtree and keeps any other", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity a = edit.createNamed("A");
    const Entity b = edit.createNamed("B", a);
    const Entity other = edit.createNamed("Other");

    SECTION("the selected entity itself") {
        edit.session.setSelected(b);
        DestroyEntityCommand destroy{edit.session, b};
        destroy.apply(edit.scene);

        CHECK_FALSE(edit.snapshot().selected.valid());
    }

    SECTION("an ancestor of the selected entity") {
        edit.session.setSelected(b);
        DestroyEntityCommand destroy{edit.session, a};
        destroy.apply(edit.scene);

        CHECK_FALSE(edit.snapshot().selected.valid());
    }

    SECTION("an entity outside the selected one's ancestry") {
        edit.session.setSelected(b);
        DestroyEntityCommand destroy{edit.session, other};
        destroy.apply(edit.scene);

        CHECK(edit.snapshot().selected == b);
    }
}

TEST_CASE("destroy of an entity that is not alive changes nothing", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity dead = edit.createNamed("Dead");
    const Entity alive = edit.createNamed("Alive");
    REQUIRE(edit.scene.destroyEntity(dead));
    edit.session.setSelected(alive);
    const EditorSnapshot before = edit.snapshot();

    SECTION("a destroyed entity") {
        DestroyEntityCommand destroy{edit.session, dead};
        destroy.apply(edit.scene);
    }

    SECTION("the empty entity") {
        DestroyEntityCommand destroy{edit.session, Entity{}};
        destroy.apply(edit.scene);
    }

    const EditorSnapshot after = edit.snapshot();
    REQUIRE(after.hierarchy.size() == 1);
    CHECK(rowFor(after, alive).name == "Alive");
    CHECK(after.selected == alive);
    CHECK(after.editRevision == before.editRevision);
}

TEST_CASE("reparent moves an entity under a new parent at the given position", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity parent = edit.createNamed("Parent");
    const Entity first = edit.createNamed("First", parent);
    const Entity last = edit.createNamed("Last", parent);
    const Entity moved = edit.createNamed("Moved");
    const EditorSnapshot before = edit.snapshot();

    ReparentEntityCommand reparent{edit.session, moved, parent, 1};
    reparent.apply(edit.scene);

    CHECK(edit.scene.getChildren(parent) == std::vector<Entity>{first, moved, last});
    CHECK(edit.scene.getRoots() == std::vector<Entity>{parent});
    const EditorSnapshot after = edit.snapshot();
    CHECK(rowFor(after, parent).childCount == 3);
    CHECK(rowFor(after, moved).depth == 1);
    CHECK(after.editRevision > before.editRevision);
}

TEST_CASE("reparent with no parent makes the entity a root and keeps its own subtree", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity parent = edit.createNamed("Parent");
    const Entity child = edit.createNamed("Child", parent);
    const Entity grandchild = edit.createNamed("Grandchild", child);
    const EditorSnapshot before = edit.snapshot();

    ReparentEntityCommand reparent{edit.session, child, Entity{}, 0};
    reparent.apply(edit.scene);

    CHECK_FALSE(edit.scene.getParent(child).valid());
    CHECK(edit.scene.getParent(grandchild) == child);
    CHECK(edit.scene.getRoots().size() == 2);
    const EditorSnapshot after = edit.snapshot();
    CHECK(rowFor(after, parent).childCount == 0);
    CHECK(rowFor(after, child).depth == 0);
    CHECK(rowFor(after, grandchild).depth == 1);
    CHECK(after.editRevision > before.editRevision);
}

TEST_CASE("reparent to the current parent reorders, counting the position among the other children", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity parent = edit.createNamed("Parent");
    const Entity first = edit.createNamed("First", parent);
    const Entity second = edit.createNamed("Second", parent);
    const Entity third = edit.createNamed("Third", parent);
    const EditorSnapshot before = edit.snapshot();

    SECTION("moving down one place") {
        ReparentEntityCommand reparent{edit.session, first, parent, 1};
        reparent.apply(edit.scene);

        CHECK(edit.scene.getChildren(parent) == std::vector<Entity>{second, first, third});
    }

    SECTION("moving to the end") {
        ReparentEntityCommand reparent{edit.session, first, parent, 2};
        reparent.apply(edit.scene);

        CHECK(edit.scene.getChildren(parent) == std::vector<Entity>{second, third, first});
    }

    SECTION("moving up to the top") {
        ReparentEntityCommand reparent{edit.session, third, parent, 0};
        reparent.apply(edit.scene);

        CHECK(edit.scene.getChildren(parent) == std::vector<Entity>{third, first, second});
    }

    const EditorSnapshot after = edit.snapshot();
    CHECK(rowFor(after, parent).childCount == 3);
    CHECK(after.editRevision > before.editRevision);
}

TEST_CASE("reparent with no parent keeps the entity where it is in the world", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity parent = edit.createNamed("Parent");
    const Entity child = edit.createNamed("Child", parent);
    TransformValues parentLocal;
    parentLocal.position = Vec3(5.0f, 0.0f, 0.0f);
    REQUIRE(edit.scene.getComponent<Transform>(parent).setLocal(parentLocal));
    TransformValues childLocal;
    childLocal.position = Vec3(1.0f, 2.0f, 0.0f);
    REQUIRE(edit.scene.getComponent<Transform>(child).setLocal(childLocal));

    ReparentEntityCommand reparent{edit.session, child, Entity{}, 0};
    reparent.apply(edit.scene);

    REQUIRE_FALSE(edit.scene.getParent(child).valid());
    const Vec3 position = edit.scene.getComponent<Transform>(child).getLocal().position;
    CHECK(position.x == Catch::Approx(6.0f).margin(1e-5));
    CHECK(position.y == Catch::Approx(2.0f).margin(1e-5));
    CHECK(position.z == Catch::Approx(0.0f).margin(1e-5));
}

TEST_CASE("reparent that the scene refuses changes nothing", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity a = edit.createNamed("A");
    const Entity b = edit.createNamed("B", a);
    const Entity c = edit.createNamed("C", b);
    const Entity dead = edit.createNamed("Dead");
    REQUIRE(edit.scene.destroyEntity(dead));
    const EditorSnapshot before = edit.snapshot();

    SECTION("onto itself") {
        ReparentEntityCommand reparent{edit.session, a, a, 0};
        reparent.apply(edit.scene);
    }

    SECTION("onto its own descendant") {
        ReparentEntityCommand reparent{edit.session, a, c, 0};
        reparent.apply(edit.scene);
    }

    SECTION("onto a dead parent") {
        ReparentEntityCommand reparent{edit.session, c, dead, 0};
        reparent.apply(edit.scene);
    }

    SECTION("a dead child") {
        ReparentEntityCommand reparent{edit.session, dead, a, 0};
        reparent.apply(edit.scene);
    }

    SECTION("the empty entity as the child") {
        ReparentEntityCommand reparent{edit.session, Entity{}, a, 0};
        reparent.apply(edit.scene);
    }

    SECTION("a root moved to the root") {
        ReparentEntityCommand reparent{edit.session, a, Entity{}, 0};
        reparent.apply(edit.scene);
    }

    CHECK(edit.scene.getRoots() == std::vector<Entity>{a});
    CHECK(edit.scene.getParent(b) == a);
    CHECK(edit.scene.getParent(c) == b);
    const EditorSnapshot after = edit.snapshot();
    CHECK(after.hierarchy.size() == 3);
    CHECK(after.editRevision == before.editRevision);
}

TEST_CASE("rename changes only that entity's name and raises the edit revision", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity renamed = edit.createNamed("Old");
    const Entity other = edit.createNamed("Old");
    const EditorSnapshot before = edit.snapshot();

    RenameEntityCommand rename{edit.session, renamed, "New"};
    rename.apply(edit.scene);

    const EditorSnapshot after = edit.snapshot();
    CHECK(rowFor(after, renamed).name == "New");
    CHECK(rowFor(after, other).name == "Old");
    CHECK(after.editRevision > before.editRevision);
}

TEST_CASE("rename accepts an empty name and a name another entity already has", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity first = edit.createNamed("Taken");
    const Entity second = edit.createNamed("Second");

    SECTION("an empty name") {
        RenameEntityCommand rename{edit.session, second, ""};
        rename.apply(edit.scene);

        CHECK(rowFor(edit.snapshot(), second).name.empty());
    }

    SECTION("a repeated name") {
        RenameEntityCommand rename{edit.session, second, "Taken"};
        rename.apply(edit.scene);

        CHECK(rowFor(edit.snapshot(), second).name == "Taken");
        CHECK(rowFor(edit.snapshot(), first).name == "Taken");
    }
}

TEST_CASE("rename to the current name keeps the edit revision", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("Same");
    const EditorSnapshot before = edit.snapshot();

    RenameEntityCommand rename{edit.session, entity, "Same"};
    rename.apply(edit.scene);

    CHECK(edit.snapshot().editRevision == before.editRevision);
}

TEST_CASE("rename of an entity that is not alive changes nothing", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity dead = edit.createNamed("Dead");
    const Entity alive = edit.createNamed("Alive");
    REQUIRE(edit.scene.destroyEntity(dead));
    const EditorSnapshot before = edit.snapshot();

    SECTION("a destroyed entity") {
        RenameEntityCommand rename{edit.session, dead, "New"};
        rename.apply(edit.scene);
    }

    SECTION("the empty entity") {
        RenameEntityCommand rename{edit.session, Entity{}, "New"};
        rename.apply(edit.scene);
    }

    const EditorSnapshot after = edit.snapshot();
    REQUIRE(after.hierarchy.size() == 1);
    CHECK(rowFor(after, alive).name == "Alive");
    CHECK(after.editRevision == before.editRevision);
}

TEST_CASE("select publishes the entity and keeps the edit revision", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity a = edit.createNamed("A");
    const Entity b = edit.createNamed("B");
    const EditorSnapshot before = edit.snapshot();

    SelectEntityCommand selectA{edit.session, a};
    selectA.apply(edit.scene);
    CHECK(edit.snapshot().selected == a);

    SelectEntityCommand selectB{edit.session, b};
    selectB.apply(edit.scene);

    const EditorSnapshot after = edit.snapshot();
    CHECK(after.selected == b);
    CHECK(after.editRevision == before.editRevision);
    CHECK(after.hierarchy.size() == before.hierarchy.size());
}

TEST_CASE("select of the empty entity clears the selection", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity a = edit.createNamed("A");
    edit.session.setSelected(a);

    SelectEntityCommand clear{edit.session, Entity{}};
    clear.apply(edit.scene);

    CHECK_FALSE(edit.snapshot().selected.valid());
}

TEST_CASE("select of a dead entity keeps the previous selection", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity alive = edit.createNamed("Alive");
    const Entity dead = edit.createNamed("Dead");
    REQUIRE(edit.scene.destroyEntity(dead));
    edit.session.setSelected(alive);
    const EditorSnapshot before = edit.snapshot();

    SelectEntityCommand select{edit.session, dead};
    select.apply(edit.scene);

    const EditorSnapshot after = edit.snapshot();
    CHECK(after.selected == alive);
    CHECK(after.editRevision == before.editRevision);
}
