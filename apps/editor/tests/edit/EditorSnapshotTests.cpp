#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/texture/Texture.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/ModelImporter.hpp>
#include <assets/TextureImporter.hpp>
#include <edit/CreateMaterialCommand.hpp>
#include <edit/EditorSnapshot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
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

TEST_CASE("the names are mixed only when two selected entities' names differ", "[editor][edit]") {
    EditorSnapshot snapshot = threeSiblings();
    snapshot.hierarchy[0].name = "Parent";
    snapshot.hierarchy[1].name = "Same";
    snapshot.hierarchy[2].name = "Same";
    snapshot.hierarchy[3].name = "Other";

    SECTION("two with the same name, beside an unselected parent named otherwise") {
        snapshot.selection = {FIRST, SECOND};

        CHECK_FALSE(snapshot.hasMixedNames());
    }

    SECTION("one name differs") {
        snapshot.selection = {FIRST, SECOND, THIRD};

        CHECK(snapshot.hasMixedNames());
    }

    SECTION("the differing name comes first") {
        snapshot.selection = {THIRD, FIRST};

        CHECK(snapshot.hasMixedNames());
    }

    SECTION("an empty name and another name") {
        snapshot.hierarchy[1].name.clear();
        snapshot.selection = {FIRST, SECOND};

        CHECK(snapshot.hasMixedNames());
    }

    SECTION("one selected") {
        snapshot.selection = {THIRD};

        CHECK_FALSE(snapshot.hasMixedNames());
    }

    SECTION("no selection") {
        snapshot.selection.clear();

        CHECK_FALSE(snapshot.hasMixedNames());
    }
}

static TechEngine::AssetRecord assetRecord(const std::uint64_t id, const TechEngine::StringId type, const std::string_view path) {
    TechEngine::AssetRecord record;
    record.id = TechEngine::ResourceId{id};
    record.typeTag = type;
    record.path = std::string(path);
    return record;
}

TEST_CASE("the picker's assets are the records of the field's type, in index order", "[editor][edit]") {
    const TechEngine::StringId mesh("TechEngine.Mesh");
    const TechEngine::StringId scene("TechEngine.Scene");
    EditorSnapshot snapshot;
    snapshot.assets = {
        assetRecord(1, mesh, "assets/client/A.temesh"),
        assetRecord(2, scene, "assets/common/Startup.tescene"),
        assetRecord(3, mesh, "assets/client/B.temesh"),
    };
    std::vector<std::size_t> out = {99};

    snapshot.collectAssetsOfType(mesh, out);

    CHECK(out == std::vector<std::size_t>{0, 2});
}

TEST_CASE("the picker leaves out a server asset of the field's type", "[editor][edit]") {
    const TechEngine::StringId mesh("TechEngine.Mesh");
    EditorSnapshot snapshot;
    snapshot.assets = {
        assetRecord(1, mesh, "assets/client/A.temesh"),
        assetRecord(2, mesh, "assets/server/Collision.temesh"),
    };
    snapshot.assets[1].side = TechEngine::Side::Server;
    std::vector<std::size_t> out;

    snapshot.collectAssetsOfType(mesh, out);

    CHECK(out == std::vector<std::size_t>{0});
}

TEST_CASE("the picker offers nothing for a type no record has, or for an empty type", "[editor][edit]") {
    EditorSnapshot snapshot;
    snapshot.assets = {assetRecord(1, TechEngine::StringId("TechEngine.Mesh"), "assets/client/A.temesh")};
    std::vector<std::size_t> out = {99};

    SECTION("a type no record has") {
        snapshot.collectAssetsOfType(TechEngine::StringId("TechEngine.Texture"), out);
    }

    SECTION("an empty type") {
        snapshot.collectAssetsOfType(TechEngine::StringId{}, out);
    }

    CHECK(out.empty());
}

static EditorSnapshot droppableAssets() {
    const TechEngine::StringId mesh("TechEngine.Mesh");
    EditorSnapshot snapshot;
    snapshot.assets = {
        assetRecord(1, mesh, "assets/client/A.temesh"),
        assetRecord(2, TechEngine::StringId("TechEngine.Material"), "assets/client/Paint.tematerial"),
        assetRecord(3, mesh, "assets/server/Collision.temesh"),
        assetRecord(4, mesh, "assets/client/B.temesh"),
    };
    snapshot.assets[2].side = TechEngine::Side::Server;
    return snapshot;
}

TEST_CASE("an asset field takes a dropped asset of its own type", "[editor][edit]") {
    const EditorSnapshot snapshot = droppableAssets();
    const std::vector<std::string> paths = {"assets/client/B.temesh"};
    TechEngine::ResourceId out;

    CHECK(snapshot.findDroppedAsset(paths, TechEngine::StringId("TechEngine.Mesh"), out));
    CHECK(out == TechEngine::ResourceId{4});
}

TEST_CASE("an asset field refuses a dropped asset the picker would not offer", "[editor][edit]") {
    const EditorSnapshot snapshot = droppableAssets();
    TechEngine::StringId type("TechEngine.Mesh");
    std::vector<std::string> paths;

    SECTION("another type") {
        paths = {"assets/client/Paint.tematerial"};
    }

    SECTION("a server asset of the field's type") {
        paths = {"assets/server/Collision.temesh"};
    }

    SECTION("a path no record holds") {
        paths = {"assets/client/Gone.temesh"};
    }

    SECTION("a source file") {
        paths = {"assets/client/Kart.gltf"};
    }

    SECTION("a field with an empty type") {
        paths = {"assets/client/A.temesh"};
        type = TechEngine::StringId{};
    }

    TechEngine::ResourceId out;
    CHECK_FALSE(snapshot.findDroppedAsset(paths, type, out));
}

TEST_CASE("an asset field refuses a drop of several assets, even when each is of its type", "[editor][edit]") {
    const EditorSnapshot snapshot = droppableAssets();
    const TechEngine::StringId mesh("TechEngine.Mesh");
    TechEngine::ResourceId out;

    SECTION("two of its type") {
        const std::vector<std::string> paths = {"assets/client/A.temesh", "assets/client/B.temesh"};
        CHECK_FALSE(snapshot.findDroppedAsset(paths, mesh, out));
    }

    SECTION("one of its type beside another type") {
        const std::vector<std::string> paths = {"assets/client/A.temesh", "assets/client/Paint.tematerial"};
        CHECK_FALSE(snapshot.findDroppedAsset(paths, mesh, out));
    }

    SECTION("the same path twice") {
        const std::vector<std::string> paths = {"assets/client/A.temesh", "assets/client/A.temesh"};
        CHECK_FALSE(snapshot.findDroppedAsset(paths, mesh, out));
    }
}

TEST_CASE("an asset field refuses a drop of no path", "[editor][edit]") {
    const EditorSnapshot snapshot = droppableAssets();
    TechEngine::ResourceId out;

    CHECK_FALSE(snapshot.findDroppedAsset({}, TechEngine::StringId("TechEngine.Mesh"), out));
}

static constexpr std::string_view COMMON_FOLDER = "project://assets/common";
static constexpr std::string_view CLIENT_FOLDER = "project://assets/client";
static constexpr std::string_view SERVER_FOLDER = "project://assets/server";

static TechEngine::ListedEntry listed(const std::string_view path, const TechEngine::ListedKind kind) {
    TechEngine::ListedEntry entry;
    entry.path = std::string(path);
    entry.side = TechEngine::Side::Client;
    entry.kind = kind;
    return entry;
}

static std::string underClient(const std::string_view relative) {
    return std::string(CLIENT_FOLDER) + "/" + std::string(relative);
}

// A client side whose byte order and name order disagree: "Sponza-old" sorts between "Sponza" and "Sponza/...", and
// capitals sort before lower case.
static EditorSnapshot browserSnapshot() {
    using TechEngine::ListedKind;
    EditorSnapshot snapshot;
    snapshot.sideFolders = {
        {TechEngine::Side::Common, std::string(COMMON_FOLDER)},
        {TechEngine::Side::Client, std::string(CLIENT_FOLDER)},
        {TechEngine::Side::Server, std::string(SERVER_FOLDER)},
    };
    snapshot.listing = {
        listed(underClient("Box.temesh"), ListedKind::Asset),
        listed(underClient("Sponza"), ListedKind::Folder),
        listed(underClient("Sponza-old"), ListedKind::Folder),
        listed(underClient("Sponza-old/Sponza.temodel"), ListedKind::Asset),
        listed(underClient("Sponza/Meshes"), ListedKind::Folder),
        listed(underClient("Sponza/Meshes/Wall.temesh"), ListedKind::Asset),
        listed(underClient("Sponza/Sponza.temodel"), ListedKind::Asset),
        listed(underClient("Zeta"), ListedKind::Folder),
        listed(underClient("alpha"), ListedKind::Folder),
        listed(underClient("readme.txt"), ListedKind::Other),
        listed(underClient("sky.hdr"), ListedKind::SkySource),
        listed(std::string(COMMON_FOLDER) + "/Shared.tematerial", ListedKind::Asset),
    };
    std::ranges::sort(snapshot.listing, {}, &TechEngine::ListedEntry::path);
    snapshot.listingRevision = 1;
    return snapshot;
}

static std::vector<std::string> pathsOf(const EditorSnapshot& snapshot, const std::vector<std::size_t>& indices) {
    std::vector<std::string> paths;
    for (const std::size_t index: indices) {
        REQUIRE(index < snapshot.listing.size());
        paths.push_back(snapshot.listing[index].path);
    }
    return paths;
}

TEST_CASE("a folder lists its own children, folders first, then files, each by name without case", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = browserSnapshot();
    std::vector<std::size_t> contents;

    snapshot.collectFolderContents(CLIENT_FOLDER, contents);

    CHECK(
        pathsOf(snapshot, contents) == std::vector<std::string>{
                                           underClient("alpha"),
                                           underClient("Sponza"),
                                           underClient("Sponza-old"),
                                           underClient("Zeta"),
                                           underClient("Box.temesh"),
                                           underClient("readme.txt"),
                                           underClient("sky.hdr"),
                                       });
}

TEST_CASE("a folder's contents leave out its subfolders' entries and a sibling whose name starts with its own", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = browserSnapshot();
    std::vector<std::size_t> contents;

    snapshot.collectFolderContents(underClient("Sponza"), contents);

    CHECK(pathsOf(snapshot, contents) == std::vector<std::string>{underClient("Sponza/Meshes"), underClient("Sponza/Sponza.temodel")});
}

TEST_CASE("an empty folder, an empty side, a file and an unlisted folder have no contents", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = browserSnapshot();
    std::vector<std::size_t> contents = {99};

    SECTION("an empty folder") {
        snapshot.collectFolderContents(underClient("alpha"), contents);
    }

    SECTION("an empty side") {
        snapshot.collectFolderContents(SERVER_FOLDER, contents);
    }

    SECTION("a file") {
        snapshot.collectFolderContents(underClient("Box.temesh"), contents);
    }

    SECTION("a folder the listing does not hold") {
        snapshot.collectFolderContents(underClient("Gone"), contents);
    }

    CHECK(contents.empty());
}

TEST_CASE("a side's contents are only that side's entries", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = browserSnapshot();
    std::vector<std::size_t> contents;

    snapshot.collectFolderContents(COMMON_FOLDER, contents);

    CHECK(pathsOf(snapshot, contents) == std::vector<std::string>{std::string(COMMON_FOLDER) + "/Shared.tematerial"});
}

TEST_CASE("the nearest folder of a listed folder or a side folder is the folder itself", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = browserSnapshot();
    std::string nearest;

    SECTION("a listed folder") {
        REQUIRE(snapshot.findNearestFolder(underClient("Sponza/Meshes"), nearest));
        CHECK(nearest == underClient("Sponza/Meshes"));
    }

    SECTION("an empty side folder") {
        REQUIRE(snapshot.findNearestFolder(SERVER_FOLDER, nearest));
        CHECK(nearest == SERVER_FOLDER);
    }
}

TEST_CASE("a folder that is gone resolves to its closest ancestor that is still listed", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = browserSnapshot();
    std::string nearest;

    SECTION("a listed parent") {
        REQUIRE(snapshot.findNearestFolder(underClient("Sponza/Meshes/Old/Deeper"), nearest));
        CHECK(nearest == underClient("Sponza/Meshes"));
    }

    SECTION("only the side is left") {
        REQUIRE(snapshot.findNearestFolder(underClient("Gone/Deeper"), nearest));
        CHECK(nearest == CLIENT_FOLDER);
    }

    SECTION("a listed file is not a folder") {
        REQUIRE(snapshot.findNearestFolder(underClient("Sponza/Sponza.temodel"), nearest));
        CHECK(nearest == underClient("Sponza"));
    }
}

TEST_CASE("a folder under no side folder has no nearest folder", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = browserSnapshot();
    std::string nearest = "stale";

    SECTION("a name that only starts like a side") {
        CHECK_FALSE(snapshot.findNearestFolder("project://assets/clientele/Sponza", nearest));
    }

    SECTION("the assets root above the sides") {
        CHECK_FALSE(snapshot.findNearestFolder("project://assets", nearest));
    }

    SECTION("another mount") {
        CHECK_FALSE(snapshot.findNearestFolder("engine://shaders", nearest));
    }

    SECTION("an empty path") {
        CHECK_FALSE(snapshot.findNearestFolder("", nearest));
    }

    CHECK(nearest.empty());
}

TEST_CASE("the breadcrumbs run from the side folder down to the folder", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = browserSnapshot();
    std::vector<std::string> crumbs = {"stale"};

    SECTION("a nested folder") {
        snapshot.collectBreadcrumbs(underClient("Sponza/Meshes"), crumbs);

        CHECK(crumbs == std::vector<std::string>{std::string(CLIENT_FOLDER), underClient("Sponza"), underClient("Sponza/Meshes")});
    }

    SECTION("a side folder") {
        snapshot.collectBreadcrumbs(COMMON_FOLDER, crumbs);

        CHECK(crumbs == std::vector<std::string>{std::string(COMMON_FOLDER)});
    }
}

TEST_CASE("a folder under no side folder has no breadcrumbs", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = browserSnapshot();
    std::vector<std::string> crumbs = {"stale"};

    snapshot.collectBreadcrumbs("project://assets/clientele/Sponza", crumbs);

    CHECK(crumbs.empty());
}

static std::string sponzaFolder(const std::uint32_t suffix) {
    return TechEngine::ModelImporter::importFolder(underClient("Sponza.gltf"), "", suffix);
}

TEST_CASE("a free name is the source's own name when nothing holds it", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = browserSnapshot();

    CHECK(snapshot.firstFreePath([](const std::uint32_t suffix) {
        return TechEngine::ModelImporter::importFolder(underClient("Bistro.fbx"), "", suffix);
    }) == underClient("Bistro"));
}

TEST_CASE("a free name skips a listed folder", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = browserSnapshot();

    CHECK(snapshot.firstFreePath(&sponzaFolder) == underClient("Sponza_1"));
}

TEST_CASE("a listed name that only starts with a candidate does not take it", "[editor][edit][browser]") {
    using TechEngine::ListedKind;
    EditorSnapshot snapshot = browserSnapshot();
    snapshot.listing.push_back(listed(underClient("Sponza_10"), ListedKind::Folder));
    snapshot.listing.push_back(listed(underClient("Sponza_1.temodel"), ListedKind::Asset));
    std::ranges::sort(snapshot.listing, {}, &TechEngine::ListedEntry::path);

    CHECK(snapshot.firstFreePath(&sponzaFolder) == underClient("Sponza_1"));
}

TEST_CASE("a free name fills the first gap among the taken suffixes", "[editor][edit][browser]") {
    using TechEngine::ListedKind;
    EditorSnapshot snapshot = browserSnapshot();
    snapshot.listing.push_back(listed(underClient("Sponza_2"), ListedKind::Folder));
    snapshot.listing.push_back(listed(underClient("Sponza_3"), ListedKind::Folder));
    std::ranges::sort(snapshot.listing, {}, &TechEngine::ListedEntry::path);

    CHECK(snapshot.firstFreePath(&sponzaFolder) == underClient("Sponza_1"));

    snapshot.listing.push_back(listed(underClient("Sponza_1"), ListedKind::Folder));
    std::ranges::sort(snapshot.listing, {}, &TechEngine::ListedEntry::path);

    CHECK(snapshot.firstFreePath(&sponzaFolder) == underClient("Sponza_4"));
}

TEST_CASE("a listed file takes a folder's name, because a create cannot make a folder over it", "[editor][edit][browser]") {
    using TechEngine::ListedKind;
    EditorSnapshot snapshot = browserSnapshot();
    snapshot.listing.push_back(listed(underClient("Crate"), ListedKind::Other));
    std::ranges::sort(snapshot.listing, {}, &TechEngine::ListedEntry::path);

    CHECK(snapshot.firstFreePath([](const std::uint32_t suffix) {
        return TechEngine::ModelImporter::importFolder(underClient("Crate.obj"), "", suffix);
    }) == underClient("Crate_1"));
}

TEST_CASE("a free sky name follows the sky importer's rule beside a listed sky", "[editor][edit][browser]") {
    using TechEngine::ListedKind;
    EditorSnapshot snapshot = browserSnapshot();
    snapshot.listing.push_back(listed(underClient("sky.tetexture"), ListedKind::Asset));
    std::ranges::sort(snapshot.listing, {}, &TechEngine::ListedEntry::path);
    const auto skyPath = [](const std::uint32_t suffix) {
        return TechEngine::TextureImporter::skyOutputPath(underClient("sky.hdr"), "", suffix);
    };

    CHECK(snapshot.firstFreePath(skyPath) == underClient("sky_1.tetexture"));
}

TEST_CASE("a name on another side is free on this one", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = browserSnapshot();

    CHECK(snapshot.firstFreePath([](const std::uint32_t suffix) {
        return TechEngine::ModelImporter::importFolder(std::string(COMMON_FOLDER) + "/Sponza.gltf", "", suffix);
    }) == std::string(COMMON_FOLDER) + "/Sponza");
}

TEST_CASE("an empty listing leaves every name free", "[editor][edit][browser]") {
    const EditorSnapshot snapshot;

    CHECK(snapshot.firstFreePath(&sponzaFolder) == underClient("Sponza"));
}

TEST_CASE("a side folder is found for itself and for anything under it", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = browserSnapshot();
    TechEngine::SideFolder side;

    REQUIRE(snapshot.findSideFolder(CLIENT_FOLDER, side));
    CHECK(side.side == TechEngine::Side::Client);
    CHECK(side.path == CLIENT_FOLDER);
    REQUIRE(snapshot.findSideFolder(underClient("Sponza/Meshes"), side));
    CHECK(side.side == TechEngine::Side::Client);
    REQUIRE(snapshot.findSideFolder(std::string(SERVER_FOLDER) + "/Unlisted", side));
    CHECK(side.side == TechEngine::Side::Server);
    CHECK(side.path == SERVER_FOLDER);
}

TEST_CASE("a path under no side folder, or one that only starts with a side folder's name, has no side", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = browserSnapshot();
    TechEngine::SideFolder side;

    CHECK_FALSE(snapshot.findSideFolder("project://assets/clientele/Sponza", side));
    CHECK_FALSE(snapshot.findSideFolder("project://assets", side));
    CHECK_FALSE(snapshot.findSideFolder("", side));
    CHECK_FALSE(EditorSnapshot{}.findSideFolder(CLIENT_FOLDER, side));
}

TEST_CASE("the longest side folder wins when side folders nest", "[editor][edit][browser]") {
    EditorSnapshot snapshot = browserSnapshot();
    snapshot.sideFolders.push_back({TechEngine::Side::Server, underClient("Nested")});
    TechEngine::SideFolder side;

    REQUIRE(snapshot.findSideFolder(underClient("Nested/Deep"), side));
    CHECK(side.side == TechEngine::Side::Server);
    REQUIRE(snapshot.findSideFolder(underClient("Sponza"), side));
    CHECK(side.side == TechEngine::Side::Client);
}

TEST_CASE("a free material name follows the material's rule beside a listed material", "[editor][edit][browser]") {
    using TechEngine::ListedKind;
    EditorSnapshot snapshot = browserSnapshot();
    snapshot.listing.push_back(listed(underClient("Material.tematerial"), ListedKind::Asset));
    std::ranges::sort(snapshot.listing, {}, &TechEngine::ListedEntry::path);

    CHECK(snapshot.firstFreePath([](const std::uint32_t suffix) {
        return TechEngine::CreateMaterialCommand::outputPath(CLIENT_FOLDER, "Material", suffix);
    }) == underClient("Material_1.tematerial"));
}

static TechEngine::ListedEntry listedAsset(const std::string_view path, const TechEngine::StringId type) {
    TechEngine::ListedEntry entry = listed(path, TechEngine::ListedKind::Asset);
    entry.typeTag = type;
    return entry;
}

static EditorSnapshot placeableListing() {
    using TechEngine::ListedKind;
    EditorSnapshot snapshot = browserSnapshot();
    snapshot.listing.push_back(listedAsset(underClient("Kart/Kart.temodel"), TechEngine::MODEL_TYPE_TAG));
    snapshot.listing.push_back(listedAsset(underClient("Front/Front.temesh"), TechEngine::MESH_TYPE_TAG));
    snapshot.listing.push_back(listedAsset(underClient("Paint.tematerial"), TechEngine::MATERIAL_TYPE_TAG));
    snapshot.listing.push_back(listedAsset(underClient("Sky.tetexture"), TechEngine::TEXTURE_TYPE_TAG));
    snapshot.listing.push_back(listedAsset(underClient("Startup.tescene"), TechEngine::SCENE_TYPE_TAG));
    snapshot.listing.push_back(listed(underClient("Kart.gltf"), ListedKind::ModelSource));
    snapshot.listing.push_back(listed(underClient("Sky.hdr"), ListedKind::SkySource));
    // An indexed file the index dropped as a duplicate is listed as Other, whatever its extension.
    snapshot.listing.push_back(listed(underClient("Copy.temodel"), ListedKind::Other));
    std::ranges::sort(snapshot.listing, {}, &TechEngine::ListedEntry::path);
    return snapshot;
}

TEST_CASE("only a listed model or mesh is placeable", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = placeableListing();

    for (const TechEngine::ListedEntry& entry: snapshot.listing) {
        const bool expected = entry.path == underClient("Kart/Kart.temodel") || entry.path == underClient("Front/Front.temesh");
        CHECK(EditorSnapshot::isPlaceable(entry) == expected);
    }
}

TEST_CASE("a drop places its models and meshes in the order given, and skips everything else", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = placeableListing();
    const std::vector<std::string> paths = {
        underClient("Front/Front.temesh"),
        underClient("Paint.tematerial"),
        underClient("Kart.gltf"),
        underClient("Kart/Kart.temodel"),
        underClient("Sponza"),
        underClient("Copy.temodel"),
        underClient("Gone.temodel"),
    };
    std::vector<std::string> out = {"stale"};

    snapshot.collectPlaceable(paths, out);

    CHECK(out == std::vector<std::string>{underClient("Front/Front.temesh"), underClient("Kart/Kart.temodel")});
}

TEST_CASE("a drop of the same model twice places it twice", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = placeableListing();
    const std::vector<std::string> paths = {underClient("Kart/Kart.temodel"), underClient("Kart/Kart.temodel")};
    std::vector<std::string> out;

    snapshot.collectPlaceable(paths, out);

    CHECK(out == paths);
}

TEST_CASE("a drop with nothing placeable places nothing", "[editor][edit][browser]") {
    const EditorSnapshot snapshot = placeableListing();
    std::vector<std::string> out = {"stale"};

    SECTION("only other kinds") {
        const std::vector<std::string> paths = {underClient("Sky.tetexture"), underClient("Sky.hdr"), underClient("Startup.tescene")};
        snapshot.collectPlaceable(paths, out);
    }

    SECTION("no path") {
        snapshot.collectPlaceable({}, out);
    }

    SECTION("a model path on an empty listing") {
        const std::vector<std::string> paths = {underClient("Kart/Kart.temodel")};
        EditorSnapshot{}.collectPlaceable(paths, out);
    }

    CHECK(out.empty());
}
