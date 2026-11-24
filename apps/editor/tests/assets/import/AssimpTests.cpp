#include <assimp/Importer.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("assimp reads glTF, FBX and OBJ", "[editor][assets][assimp]") {
    const Assimp::Importer importer;
    CHECK(importer.IsExtensionSupported(".gltf"));
    CHECK(importer.IsExtensionSupported(".glb"));
    CHECK(importer.IsExtensionSupported(".fbx"));
    CHECK(importer.IsExtensionSupported(".obj"));
}

// glTF brings two importers, one for glTF 1.0 and one for 2.0.
TEST_CASE("assimp is built with no other importer", "[editor][assets][assimp]") {
    const Assimp::Importer importer;
    CHECK(importer.GetImporterCount() == 4);
    CHECK_FALSE(importer.IsExtensionSupported(".3ds"));
    CHECK_FALSE(importer.IsExtensionSupported(".dae"));
    CHECK_FALSE(importer.IsExtensionSupported(".blend"));
    CHECK_FALSE(importer.IsExtensionSupported(".stl"));
}
