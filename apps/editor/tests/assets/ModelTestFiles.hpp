#pragma once

#include <string>
#include <string_view>

namespace TechEngineTests {
    // Every mesh in a body draws the same triangle, at (1, 2, 3), (4, 2, 3) and (1, 5, 3), through accessors 0 and 1.
    inline std::string gltfFile(const std::string_view body) {
        std::string text = R"({"asset":{"version":"2.0"},"scene":0,)";
        text += body;
        text += R"(,"accessors":[{"bufferView":0,"componentType":5123,"count":3,"type":"SCALAR"},)";
        text += R"({"bufferView":1,"componentType":5126,"count":3,"type":"VEC3","min":[1,2,3],"max":[4,5,3]}],)";
        text += R"("bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":6,"target":34963},)";
        text += R"({"buffer":0,"byteOffset":8,"byteLength":36,"target":34962}],)";
        text += R"("buffers":[{"byteLength":44,"uri":"data:application/octet-stream;base64,AAABAAIAAAAAAIA/AAAAQAAAQEAAAIBAAAAAQAAAQEAAAIA/AACgQAAAQEA="}]})";
        return text;
    }

    // Two top-level nodes, so assimp adds a root above them: Body (mesh Hull, 5 along Z) holds Wheel (mesh Tire, a
    // quarter turn about Z and a scale of 1, 2, 3), and Empty holds nothing.
    inline constexpr std::string_view GLTF_PAIR_BODY = R"("scenes":[{"nodes":[0,2]}],)"
                                                       R"("nodes":[{"name":"Body","mesh":0,"translation":[0,0,5],"children":[1]},)"
                                                       R"({"name":"Wheel","mesh":1,"rotation":[0,0,0.70710678,0.70710678],"scale":[1,2,3]},)"
                                                       R"({"name":"Empty"}],)"
                                                       R"("meshes":[{"name":"Hull","primitives":[{"attributes":{"POSITION":1},"indices":0}]},)"
                                                       R"({"name":"Tire","primitives":[{"attributes":{"POSITION":1},"indices":0}]}])";

    inline constexpr std::string_view GLTF_SHARED_MESH_BODY = R"("scenes":[{"nodes":[0,1]}],)"
                                                              R"("nodes":[{"name":"Left","mesh":0},{"name":"Right","mesh":0}],)"
                                                              R"("meshes":[{"name":"Panel","primitives":[{"attributes":{"POSITION":1},"indices":0}]}])";

    // One top-level node, which assimp makes the root, with a mesh of two primitives.
    inline constexpr std::string_view GLTF_TWO_PRIMITIVES_BODY = R"("scenes":[{"nodes":[0]}],)"
                                                                 R"("nodes":[{"name":"Car","mesh":0,"translation":[1,0,0]}],)"
                                                                 R"("meshes":[{"name":"Parts","primitives":[{"attributes":{"POSITION":1},"indices":0},{"attributes":{"POSITION":1},"indices":0}]}])";

    inline constexpr std::string_view GLTF_POINTS_AND_TRIANGLES_BODY = R"("scenes":[{"nodes":[0,1]}],)"
                                                                       R"("nodes":[{"name":"Dots","mesh":0},{"name":"Solid","mesh":1}],)"
                                                                       R"("meshes":[{"name":"Cloud","primitives":[{"attributes":{"POSITION":1},"mode":0}]},)"
                                                                       R"({"name":"Shell","primitives":[{"attributes":{"POSITION":1},"indices":0}]}])";

    inline constexpr std::string_view GLTF_POINTS_ONLY_BODY = R"("scenes":[{"nodes":[0]}],)"
                                                              R"("nodes":[{"name":"Dots","mesh":0}],)"
                                                              R"("meshes":[{"name":"Cloud","primitives":[{"attributes":{"POSITION":1},"mode":0}]}])";

    // Three names that are one file name once the slash is replaced and case is ignored.
    inline constexpr std::string_view GLTF_CLASHING_NAMES_BODY = R"("scenes":[{"nodes":[0,1,2]}],)"
                                                                 R"("nodes":[{"name":"First","mesh":0},{"name":"Second","mesh":1},{"name":"Third","mesh":2}],)"
                                                                 R"("meshes":[{"name":"Part/A","primitives":[{"attributes":{"POSITION":1},"indices":0}]},)"
                                                                 R"({"name":"Part_A","primitives":[{"attributes":{"POSITION":1},"indices":0}]},)"
                                                                 R"({"name":"part_a","primitives":[{"attributes":{"POSITION":1},"indices":0}]}])";

    inline constexpr std::string_view OBJ_QUAD = "o Quad\nv 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nf 1 2 3 4\n";
}
