#pragma once

#include <TechEngine/base/math/Math.hpp>

#include <cstdint>
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

    // Window uses material 1, and Door, under it and 10 along Z, uses material 0. assimp's glTF reader numbers the
    // materials in the order it reads them, and it reads a node's children before its mesh, so Paint stays first while a
    // walk from the root meets Glass first. No mesh uses material 2.
    inline constexpr std::string_view GLTF_TWO_MATERIALS_BODY = R"("scenes":[{"nodes":[0]}],)"
                                                                R"("nodes":[{"name":"Window","mesh":0,"children":[1]},{"name":"Door","mesh":1,"translation":[0,0,10]}],)"
                                                                R"("materials":[{"name":"Paint"},{"name":"Glass"},{"name":"Unused"}],)"
                                                                R"("meshes":[{"name":"Pane","primitives":[{"attributes":{"POSITION":1},"indices":0,"material":1}]},)"
                                                                R"({"name":"Board","primitives":[{"attributes":{"POSITION":1},"indices":0,"material":0}]}])";

    inline constexpr std::string_view GLTF_SAME_NAMED_MATERIALS_BODY = R"("scenes":[{"nodes":[0,1]}],)"
                                                                       R"("nodes":[{"name":"Window","mesh":0},{"name":"Door","mesh":1}],)"
                                                                       R"("materials":[{"name":"Paint"},{"name":"Paint"}],)"
                                                                       R"("meshes":[{"name":"Pane","primitives":[{"attributes":{"POSITION":1},"indices":0,"material":1}]},)"
                                                                       R"({"name":"Board","primitives":[{"attributes":{"POSITION":1},"indices":0,"material":0}]}])";

    // Two faces that meet at a right angle along the edge from (0, 0, 0) to (1, 0, 0), with no normals.
    inline constexpr std::string_view OBJ_BENT = "o Bent\nv 0 0 0\nv 1 0 0\nv 0 1 0\nv 0 0 1\nf 1 2 3\nf 1 4 2\n";

    inline constexpr std::string_view OBJ_QUAD = "o Quad\nv 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nf 1 2 3 4\n";

    // No normals, so they are made flat, +Z. U runs along +X and V along +Y, in OBJ's own bottom-left UV space.
    inline constexpr std::string_view OBJ_QUAD_WITH_UVS = "o Quad\nv 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\n"
                                                          "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nf 1/1 2/2 3/3 4/4\n";

    inline const TechEngine::Vec3 QUAD_NORMAL{0.0f, 0.6f, 0.8f};
    inline const TechEngine::Vec4 QUAD_TANGENT{1.0f, 0.0f, 0.0f, 1.0f};

    // A quad of two triangles sharing two corners: (1, 2, 3), (4, 2, 3), (1, 5, 3) and (4, 5, 3), indices 0 1 2 2 1 3.
    // Every corner has the normal QUAD_NORMAL and the tangent QUAD_TANGENT. Its glTF UVs are (0, 0), (1, 0), (0, 1) and
    // (1, 1), so U runs along +X and glTF's V along +Y. Accessors: 0 indices, 1 POSITION, 2 NORMAL, 3 TEXCOORD_0, 4 TANGENT.
    inline std::string gltfQuadFile(const std::string_view body) {
        std::string text = R"({"asset":{"version":"2.0"},"scene":0,)";
        text += body;
        text += R"(,"accessors":[{"bufferView":0,"componentType":5123,"count":6,"type":"SCALAR"},)";
        text += R"({"bufferView":1,"componentType":5126,"count":4,"type":"VEC3","min":[1,2,3],"max":[4,5,3]},)";
        text += R"({"bufferView":2,"componentType":5126,"count":4,"type":"VEC3"},)";
        text += R"({"bufferView":3,"componentType":5126,"count":4,"type":"VEC2"},)";
        text += R"({"bufferView":4,"componentType":5126,"count":4,"type":"VEC4"}],)";
        text += R"("bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":12,"target":34963},)";
        text += R"({"buffer":0,"byteOffset":12,"byteLength":48,"target":34962},)";
        text += R"({"buffer":0,"byteOffset":60,"byteLength":48,"target":34962},)";
        text += R"({"buffer":0,"byteOffset":108,"byteLength":32,"target":34962},)";
        text += R"({"buffer":0,"byteOffset":140,"byteLength":64,"target":34962}],)";
        text += R"("buffers":[{"byteLength":204,"uri":"data:application/octet-stream;base64,)";
        text += "AAABAAIAAgABAAMAAACAPwAAAEAAAEBAAACAQAAAAEAAAEBAAACAPwAAoEAAAEBAAACAQAAAoEAAAEBAAAAAAJqZGT/NzEw/AAAAAJqZGT/NzEw/"
                "AAAAAJqZGT/NzEw/AAAAAJqZGT/NzEw/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AACAPwAAgD8AAIA/AAAAAAAAAAAAAIA/AACAPwAAAAAAAAAA"
                "AACAPwAAgD8AAAAAAAAAAAAAgD8AAIA/AAAAAAAAAAAAAIA/";
        text += R"("}]})";
        return text;
    }

    inline constexpr std::string_view GLTF_QUAD_WITH_TANGENTS_BODY = R"("scenes":[{"nodes":[0]}],)"
                                                                     R"("nodes":[{"name":"Panel","mesh":0}],)"
                                                                     R"("meshes":[{"name":"Panel","primitives":[{"attributes":{"POSITION":1,"NORMAL":2,"TEXCOORD_0":3,"TANGENT":4},"indices":0}]}])";

    inline constexpr std::string_view GLTF_QUAD_WITHOUT_TANGENTS_BODY = R"("scenes":[{"nodes":[0]}],)"
                                                                        R"("nodes":[{"name":"Panel","mesh":0}],)"
                                                                        R"("meshes":[{"name":"Panel","primitives":[{"attributes":{"POSITION":1,"NORMAL":2,"TEXCOORD_0":3},"indices":0}]}])";

    inline constexpr std::string_view GLTF_QUAD_WITHOUT_UVS_BODY = R"("scenes":[{"nodes":[0]}],)"
                                                                   R"("nodes":[{"name":"Panel","mesh":0}],)"
                                                                   R"("meshes":[{"name":"Panel","primitives":[{"attributes":{"POSITION":1,"NORMAL":2},"indices":0}]}])";

    // One quad used by two nodes, the second mirrored by a scale of -1 on X.
    inline constexpr std::string_view GLTF_MIRRORED_QUAD_BODY = R"("scenes":[{"nodes":[0,1]}],)"
                                                                R"("nodes":[{"name":"Left","mesh":0},{"name":"Right","mesh":0,"scale":[-1,1,1]}],)"
                                                                R"("meshes":[{"name":"Panel","primitives":[{"attributes":{"POSITION":1,"NORMAL":2,"TEXCOORD_0":3,"TANGENT":4},"indices":0}]}])";

    // Stretch scales Y by 2 above Panel, which turns its quad an eighth about Z, so the scale is not along the quad's axes.
    inline constexpr std::string_view GLTF_STRETCHED_QUAD_BODY = R"("scenes":[{"nodes":[0]}],)"
                                                                 R"("nodes":[{"name":"Stretch","scale":[1,2,1],"children":[1]},)"
                                                                 R"({"name":"Panel","mesh":0,"rotation":[0,0,0.38268343,0.92387953]}],)"
                                                                 R"("meshes":[{"name":"Panel","primitives":[{"attributes":{"POSITION":1,"NORMAL":2,"TEXCOORD_0":3,"TANGENT":4},"indices":0}]}])";

    // Mapped has UV0 and Bare, 10 along Z, has none.
    inline constexpr std::string_view GLTF_QUAD_PART_WITHOUT_UVS_BODY = R"("scenes":[{"nodes":[0,1]}],)"
                                                                        R"("nodes":[{"name":"Mapped","mesh":0},{"name":"Bare","mesh":1,"translation":[0,0,10]}],)"
                                                                        R"("meshes":[{"name":"Mapped","primitives":[{"attributes":{"POSITION":1,"NORMAL":2,"TEXCOORD_0":3},"indices":0}]},)"
                                                                        R"({"name":"Bare","primitives":[{"attributes":{"POSITION":1,"NORMAL":2},"indices":0}]}])";

    inline void appendU32(std::string& out, const std::uint32_t value) {
        for (std::uint32_t shift = 0; shift < 32; shift += 8) {
            out.push_back(static_cast<char>((value >> shift) & 0xFFu));
        }
    }

    // A binary glTF with one JSON chunk and no binary chunk, so its buffers must be data URIs.
    inline std::string glbFile(const std::string_view json) {
        std::string chunk(json);
        while (chunk.size() % 4 != 0) {
            chunk.push_back(' ');
        }
        std::string file = "glTF";
        appendU32(file, 2);
        appendU32(file, static_cast<std::uint32_t>(12 + 8 + chunk.size()));
        appendU32(file, static_cast<std::uint32_t>(chunk.size()));
        file += "JSON";
        file += chunk;
        return file;
    }
}
