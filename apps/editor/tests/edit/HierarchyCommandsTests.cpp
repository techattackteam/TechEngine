#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <edit/EditRecord.hpp>
#include <edit/EditTestScene.hpp>
#include <edit/EditorSnapshot.hpp>
#include <edit/FieldValue.hpp>
#include <edit/HierarchyCommands.hpp>
#include <edit/InspectorCommands.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
#include <variant>
#include <vector>

using TechEngine::CreateEntityCommand;
using TechEngine::DestroyEntityCommand;
using TechEngine::EditorSnapshot;
using TechEngine::Entity;
using TechEngine::RenameEntityCommand;
using TechEngine::RenameRecord;
using TechEngine::ReparentEntityCommand;
using TechEngine::SelectEntityCommand;
using TechEngine::Transform;
using TechEngine::TransformValues;
using TechEngine::Vec3;
using TechEngineTests::EditTestScene;
using TechEngineTests::rowFor;

static std::vector<std::string> g_hierarchyCommandsLog;

static void captureHierarchyCommandsLog(const TechEngine::LogRecord& record) {
    g_hierarchyCommandsLog.emplace_back(record.message);
}

class HierarchyCommandsLogCapture {
private:
    TechEngine::Level m_previousLevel;

public:
    HierarchyCommandsLogCapture() : m_previousLevel(TechEngine::minLevel()) {
        g_hierarchyCommandsLog.clear();
        TechEngine::setMinLevel(TechEngine::Level::Warn);
        (void)TechEngine::addLogSink(&captureHierarchyCommandsLog);
    }

    ~HierarchyCommandsLogCapture() {
        (void)TechEngine::removeLogSink(&captureHierarchyCommandsLog);
        TechEngine::setMinLevel(m_previousLevel);
        g_hierarchyCommandsLog.clear();
    }

    HierarchyCommandsLogCapture(const HierarchyCommandsLogCapture&) = delete;

    HierarchyCommandsLogCapture& operator=(const HierarchyCommandsLogCapture&) = delete;
};

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

    DestroyEntityCommand destroy{edit.session, {b}};
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

TEST_CASE("destroy drops the destroyed entities from the selection and keeps the others", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity a = edit.createNamed("A");
    const Entity b = edit.createNamed("B", a);
    const Entity other = edit.createNamed("Other");

    SECTION("the only selected entity") {
        edit.selectOnly(b);
        DestroyEntityCommand destroy{edit.session, {b}};
        destroy.apply(edit.scene);

        const EditorSnapshot after = edit.snapshot();
        CHECK(after.selection.empty());
        CHECK_FALSE(after.active.valid());
    }

    SECTION("an ancestor of a selected entity") {
        edit.session.setSelection({b, other}, other, 1);
        DestroyEntityCommand destroy{edit.session, {a}};
        destroy.apply(edit.scene);

        const EditorSnapshot after = edit.snapshot();
        CHECK(after.selection == std::vector<Entity>{other});
        CHECK(after.active == other);
    }

    SECTION("an entity outside the selection and its ancestry") {
        edit.selectOnly(b);
        DestroyEntityCommand destroy{edit.session, {other}};
        destroy.apply(edit.scene);

        const EditorSnapshot after = edit.snapshot();
        CHECK(after.selection == std::vector<Entity>{b});
        CHECK(after.active == b);
    }
}

TEST_CASE("deleting a selected parent and its selected child is one command with no warning", "[editor][edit][commands]") {
    const HierarchyCommandsLogCapture logs;
    EditTestScene edit;
    const Entity parent = edit.createNamed("Parent");
    const Entity child = edit.createNamed("Child", parent);
    const Entity grandchild = edit.createNamed("Grandchild", child);
    const Entity other = edit.createNamed("Other");
    const Entity kept = edit.createNamed("Kept");
    edit.session.setSelection({grandchild, other, child, parent}, child, 1);
    const EditorSnapshot before = edit.snapshot();

    std::vector<Entity> topmost;
    before.collectTopmost(before.selection, topmost);
    REQUIRE(std::ranges::is_permutation(topmost, std::vector<Entity>{parent, other}));
    DestroyEntityCommand destroy{edit.session, topmost};
    destroy.apply(edit.scene);

    CHECK_FALSE(edit.scene.contains(parent));
    CHECK_FALSE(edit.scene.contains(child));
    CHECK_FALSE(edit.scene.contains(grandchild));
    CHECK_FALSE(edit.scene.contains(other));
    CHECK(edit.scene.contains(kept));
    const EditorSnapshot after = edit.snapshot();
    CHECK(after.selection.empty());
    CHECK(after.editRevision > before.editRevision);
    CHECK(g_hierarchyCommandsLog.empty());
}

TEST_CASE("destroy of an entity that is not alive changes nothing", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity dead = edit.createNamed("Dead");
    const Entity alive = edit.createNamed("Alive");
    REQUIRE(edit.scene.destroyEntity(dead));
    edit.selectOnly(alive);
    const EditorSnapshot before = edit.snapshot();

    SECTION("a destroyed entity") {
        DestroyEntityCommand destroy{edit.session, {dead}};
        destroy.apply(edit.scene);
    }

    SECTION("the empty entity") {
        DestroyEntityCommand destroy{edit.session, {Entity{}}};
        destroy.apply(edit.scene);
    }

    const EditorSnapshot after = edit.snapshot();
    REQUIRE(after.hierarchy.size() == 1);
    CHECK(rowFor(after, alive).name == "Alive");
    CHECK(after.selection == std::vector<Entity>{alive});
    CHECK(after.editRevision == before.editRevision);
}

TEST_CASE("destroy of several skips an entity that is not alive and still destroys the rest", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity first = edit.createNamed("First");
    const Entity dead = edit.createNamed("Dead");
    const Entity last = edit.createNamed("Last");
    const Entity kept = edit.createNamed("Kept");
    REQUIRE(edit.scene.destroyEntity(dead));
    edit.session.setSelection({first, last, kept}, last, 1);
    const EditorSnapshot before = edit.snapshot();

    DestroyEntityCommand destroy{edit.session, {first, dead, Entity{}, last}};
    destroy.apply(edit.scene);

    CHECK_FALSE(edit.scene.contains(first));
    CHECK_FALSE(edit.scene.contains(last));
    CHECK(edit.scene.contains(kept));
    const EditorSnapshot after = edit.snapshot();
    CHECK(after.selection == std::vector<Entity>{kept});
    CHECK_FALSE(after.active.valid());
    CHECK(after.editRevision > before.editRevision);
}

TEST_CASE("reparent moves an entity under a new parent at the given position", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity parent = edit.createNamed("Parent");
    const Entity first = edit.createNamed("First", parent);
    const Entity last = edit.createNamed("Last", parent);
    const Entity moved = edit.createNamed("Moved");
    const EditorSnapshot before = edit.snapshot();

    ReparentEntityCommand reparent{edit.session, {moved}, parent, 1};
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

    ReparentEntityCommand reparent{edit.session, {child}, Entity{}, 0};
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
        ReparentEntityCommand reparent{edit.session, {first}, parent, 1};
        reparent.apply(edit.scene);

        CHECK(edit.scene.getChildren(parent) == std::vector<Entity>{second, first, third});
    }

    SECTION("moving to the end") {
        ReparentEntityCommand reparent{edit.session, {first}, parent, 2};
        reparent.apply(edit.scene);

        CHECK(edit.scene.getChildren(parent) == std::vector<Entity>{second, third, first});
    }

    SECTION("moving up to the top") {
        ReparentEntityCommand reparent{edit.session, {third}, parent, 0};
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

    ReparentEntityCommand reparent{edit.session, {child}, Entity{}, 0};
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
        ReparentEntityCommand reparent{edit.session, {a}, a, 0};
        reparent.apply(edit.scene);
    }

    SECTION("onto its own descendant") {
        ReparentEntityCommand reparent{edit.session, {a}, c, 0};
        reparent.apply(edit.scene);
    }

    SECTION("onto a dead parent") {
        ReparentEntityCommand reparent{edit.session, {c}, dead, 0};
        reparent.apply(edit.scene);
    }

    SECTION("a dead child") {
        ReparentEntityCommand reparent{edit.session, {dead}, a, 0};
        reparent.apply(edit.scene);
    }

    SECTION("the empty entity as the child") {
        ReparentEntityCommand reparent{edit.session, {Entity{}}, a, 0};
        reparent.apply(edit.scene);
    }

    SECTION("a root moved to the root") {
        ReparentEntityCommand reparent{edit.session, {a}, Entity{}, 0};
        reparent.apply(edit.scene);
    }

    CHECK(edit.scene.getRoots() == std::vector<Entity>{a});
    CHECK(edit.scene.getParent(b) == a);
    CHECK(edit.scene.getParent(c) == b);
    const EditorSnapshot after = edit.snapshot();
    CHECK(after.hierarchy.size() == 3);
    CHECK(after.editRevision == before.editRevision);
}

TEST_CASE("a move of several under a new parent keeps their order", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity target = edit.createNamed("Target");
    const Entity existing = edit.createNamed("Existing", target);
    const Entity source = edit.createNamed("Source");
    const Entity first = edit.createNamed("First", source);
    const Entity stays = edit.createNamed("Stays", source);
    const Entity second = edit.createNamed("Second", source);
    const Entity root = edit.createNamed("Root");
    const EditorSnapshot before = edit.snapshot();

    SECTION("at the top") {
        ReparentEntityCommand reparent{edit.session, {first, second, root}, target, 0};
        reparent.apply(edit.scene);

        CHECK(edit.scene.getChildren(target) == std::vector<Entity>{first, second, root, existing});
    }

    SECTION("at the end") {
        ReparentEntityCommand reparent{edit.session, {first, second, root}, target, 1};
        reparent.apply(edit.scene);

        CHECK(edit.scene.getChildren(target) == std::vector<Entity>{existing, first, second, root});
    }

    CHECK(edit.scene.getChildren(source) == std::vector<Entity>{stays});
    CHECK(edit.snapshot().editRevision > before.editRevision);
}

TEST_CASE("a move of several inside their own parent counts the position among the children not moved", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity parent = edit.createNamed("Parent");
    const Entity a = edit.createNamed("A", parent);
    const Entity b = edit.createNamed("B", parent);
    const Entity c = edit.createNamed("C", parent);
    const Entity d = edit.createNamed("D", parent);

    SECTION("to the end") {
        ReparentEntityCommand reparent{edit.session, {a, c}, parent, 2};
        reparent.apply(edit.scene);

        CHECK(edit.scene.getChildren(parent) == std::vector<Entity>{b, d, a, c});
    }

    SECTION("between the two left behind") {
        ReparentEntityCommand reparent{edit.session, {a, c}, parent, 1};
        reparent.apply(edit.scene);

        CHECK(edit.scene.getChildren(parent) == std::vector<Entity>{b, a, c, d});
    }

    SECTION("up to the top") {
        ReparentEntityCommand reparent{edit.session, {b, d}, parent, 0};
        reparent.apply(edit.scene);

        CHECK(edit.scene.getChildren(parent) == std::vector<Entity>{b, d, a, c});
    }
}

static void flattenScaleX(EditTestScene& edit, const Entity entity) {
    TechEngine::SetFieldCommand flatten{edit.session, edit.getRegistry(), entity, TechEngine::componentTypeId<Transform>(), TechEngineTests::TRANSFORM_SCALE_X_FIELD, TechEngine::FieldValue{0.0f}};
    flatten.apply(edit.scene);
    REQUIRE(edit.scene.getComponent<Transform>(entity).getLocal().scale.x == 0.0f);
}

TEST_CASE("a move of several skips a child the scene refuses, moves the rest and raises the edit revision", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity target = edit.createNamed("Target");
    const Entity holder = edit.createNamed("Holder");
    const Entity first = edit.createNamed("First", holder);
    const Entity flat = edit.createNamed("Flat", holder);
    const Entity last = edit.createNamed("Last", holder);
    flattenScaleX(edit, flat);
    const EditorSnapshot before = edit.snapshot();

    SECTION("under a parent") {
        ReparentEntityCommand reparent{edit.session, {first, flat, last}, target, 0};
        reparent.apply(edit.scene);

        CHECK(edit.scene.getChildren(target) == std::vector<Entity>{first, last});
    }

    SECTION("to the root") {
        ReparentEntityCommand reparent{edit.session, {first, flat, last}, Entity{}, 0};
        reparent.apply(edit.scene);

        CHECK_FALSE(edit.scene.getParent(first).valid());
        CHECK_FALSE(edit.scene.getParent(last).valid());
    }

    CHECK(edit.scene.getParent(flat) == holder);
    CHECK(edit.snapshot().editRevision > before.editRevision);
}

TEST_CASE("a move of several to the root skips the entities that are already roots", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity root = edit.createNamed("Root");
    const Entity parent = edit.createNamed("Parent");
    const Entity child = edit.createNamed("Child", parent);
    const EditorSnapshot before = edit.snapshot();

    SECTION("a root beside a child moves only the child") {
        ReparentEntityCommand reparent{edit.session, {root, child}, Entity{}, 0};
        reparent.apply(edit.scene);

        CHECK_FALSE(edit.scene.getParent(child).valid());
        CHECK(edit.scene.getChildren(parent).empty());
        CHECK(edit.snapshot().editRevision > before.editRevision);
    }

    SECTION("only roots change nothing") {
        ReparentEntityCommand reparent{edit.session, {root, parent}, Entity{}, 0};
        reparent.apply(edit.scene);

        CHECK(edit.scene.getParent(child) == parent);
        CHECK(edit.snapshot().editRevision == before.editRevision);
    }

    CHECK_FALSE(edit.scene.getParent(root).valid());
    CHECK_FALSE(edit.scene.getParent(parent).valid());
}

TEST_CASE("a move into any moved subtree is refused and moves none of them", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity first = edit.createNamed("First");
    const Entity second = edit.createNamed("Second");
    const Entity inside = edit.createNamed("Inside", second);
    const Entity target = edit.createNamed("Target");
    const EditorSnapshot before = edit.snapshot();

    SECTION("under a child of the second moved entity") {
        ReparentEntityCommand reparent{edit.session, {first, second}, inside, 0};
        reparent.apply(edit.scene);
    }

    SECTION("under one of the moved entities") {
        ReparentEntityCommand reparent{edit.session, {first, second}, second, 0};
        reparent.apply(edit.scene);
    }

    CHECK_FALSE(edit.scene.getParent(first).valid());
    CHECK_FALSE(edit.scene.getParent(second).valid());
    CHECK(edit.scene.getParent(inside) == second);
    CHECK(edit.scene.getChildren(target).empty());
    CHECK(edit.snapshot().editRevision == before.editRevision);
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

TEST_CASE("rename records the name it replaces", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("Old");

    RenameEntityCommand rename{edit.session, entity, "New"};
    rename.apply(edit.scene);

    REQUIRE(edit.session.getRecords().size() == 1);
    REQUIRE(std::holds_alternative<RenameRecord>(edit.session.getRecords()[0]));
    const RenameRecord& record = std::get<RenameRecord>(edit.session.getRecords()[0]);
    CHECK(record.entity == entity);
    CHECK(record.oldName == "Old");
}

TEST_CASE("rename of an entity that is not alive records nothing", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity dead = edit.createNamed("Dead");
    REQUIRE(edit.scene.destroyEntity(dead));

    RenameEntityCommand rename{edit.session, dead, "New"};
    rename.apply(edit.scene);

    CHECK(edit.session.getRecords().empty());
}

TEST_CASE("rename accepts an empty name and a name another entity already has", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity first = edit.createNamed("Taken");
    const Entity second = edit.createNamed("Second");

    SECTION("an empty name") {
        RenameEntityCommand rename{edit.session, second, ""};
        rename.apply(edit.scene);

        const EditorSnapshot after = edit.snapshot();
        CHECK(rowFor(after, second).name.empty());
    }

    SECTION("a repeated name") {
        RenameEntityCommand rename{edit.session, second, "Taken"};
        rename.apply(edit.scene);

        const EditorSnapshot after = edit.snapshot();
        CHECK(rowFor(after, second).name == "Taken");
        CHECK(rowFor(after, first).name == "Taken");
    }
}

TEST_CASE("rename to the current name keeps the edit revision and records nothing", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity entity = edit.createNamed("Same");
    const EditorSnapshot before = edit.snapshot();

    RenameEntityCommand rename{edit.session, entity, "Same"};
    rename.apply(edit.scene);

    CHECK(edit.snapshot().editRevision == before.editRevision);
    CHECK(edit.session.getRecords().empty());
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

TEST_CASE("a select replaces the whole set and keeps the edit revision", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity a = edit.createNamed("A");
    const Entity b = edit.createNamed("B");
    const Entity c = edit.createNamed("C");
    const EditorSnapshot before = edit.snapshot();

    SelectEntityCommand selectTwo{edit.session, {a, b}, b, 1};
    selectTwo.apply(edit.scene);
    const EditorSnapshot two = edit.snapshot();
    CHECK(std::ranges::is_permutation(two.selection, std::vector<Entity>{a, b}));
    CHECK(two.active == b);
    CHECK(two.selectionSequence == 1);

    SelectEntityCommand selectOne{edit.session, {c}, c, 2};
    selectOne.apply(edit.scene);

    const EditorSnapshot after = edit.snapshot();
    CHECK(after.selection == std::vector<Entity>{c});
    CHECK(after.active == c);
    CHECK(after.selectionSequence == 2);
    CHECK(after.editRevision == before.editRevision);
    CHECK(after.hierarchy.size() == before.hierarchy.size());
}

TEST_CASE("a select with an older sequence never overwrites a newer one", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity a = edit.createNamed("A");
    const Entity b = edit.createNamed("B");

    SelectEntityCommand newer{edit.session, {a}, a, 5};
    SelectEntityCommand older{edit.session, {a, b}, b, 4};
    newer.apply(edit.scene);
    older.apply(edit.scene);

    const EditorSnapshot after = edit.snapshot();
    CHECK(after.selection == std::vector<Entity>{a});
    CHECK(after.active == a);
    CHECK(after.selectionSequence == 5);
}

TEST_CASE("a select of the empty set clears the selection", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity a = edit.createNamed("A");
    edit.selectOnly(a);

    SelectEntityCommand clear{edit.session, {}, Entity{}, 9};
    clear.apply(edit.scene);

    const EditorSnapshot after = edit.snapshot();
    CHECK(after.selection.empty());
    CHECK_FALSE(after.active.valid());
    CHECK(after.selectionSequence == 9);
}

TEST_CASE("a select leaves out a dead entity and still stores its sequence", "[editor][edit][commands]") {
    EditTestScene edit;
    const Entity alive = edit.createNamed("Alive");
    const Entity dead = edit.createNamed("Dead");
    REQUIRE(edit.scene.destroyEntity(dead));
    const EditorSnapshot before = edit.snapshot();

    SECTION("beside a live one") {
        SelectEntityCommand select{edit.session, {alive, dead}, alive, 3};
        select.apply(edit.scene);

        const EditorSnapshot after = edit.snapshot();
        CHECK(after.selection == std::vector<Entity>{alive});
        CHECK(after.active == alive);
    }

    SECTION("alone") {
        SelectEntityCommand select{edit.session, {dead}, dead, 3};
        select.apply(edit.scene);

        const EditorSnapshot after = edit.snapshot();
        CHECK(after.selection.empty());
        CHECK_FALSE(after.active.valid());
    }

    const EditorSnapshot after = edit.snapshot();
    CHECK(after.selectionSequence == 3);
    CHECK(after.editRevision == before.editRevision);
}
