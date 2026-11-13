#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>

#include <assets/MeshImporter.hpp>

#include <assimp/mesh.h>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <meshoptimizer.h>
#include <mikktspace.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace TechEngine {
    static constexpr unsigned int NOT_TRIANGLES = aiPrimitiveType_POINT | aiPrimitiveType_LINE | aiPrimitiveType_POLYGON;
    static constexpr float MIN_DIRECTION_LENGTH_SQUARED = 1.0e-12f;

    struct TangentGenerationMesh {
        const Mesh* mesh = nullptr;
        std::vector<Vec4> cornerTangents;
    };

    static Vec3 toVec3(const aiVector3D& value) {
        return Vec3(value.x, value.y, value.z);
    }

    static const TangentGenerationMesh& generationMesh(const SMikkTSpaceContext* context) {
        return *static_cast<const TangentGenerationMesh*>(context->m_pUserData);
    }

    static std::size_t cornerIndex(const int face, const int corner) {
        return static_cast<std::size_t>(face) * 3 + static_cast<std::size_t>(corner);
    }

    static std::uint32_t cornerVertex(const SMikkTSpaceContext* context, const int face, const int corner) {
        return generationMesh(context).mesh->indices[cornerIndex(face, corner)];
    }

    static int faceCount(const SMikkTSpaceContext* context) {
        return static_cast<int>(generationMesh(context).mesh->indices.size() / 3);
    }

    static int cornersOfFace(const SMikkTSpaceContext* /*context*/, const int /*face*/) {
        return 3;
    }

    static void cornerPosition(const SMikkTSpaceContext* context, float* out, const int face, const int corner) {
        const Vec3 position = generationMesh(context).mesh->positions[cornerVertex(context, face, corner)];
        out[0] = position.x;
        out[1] = position.y;
        out[2] = position.z;
    }

    static void cornerNormal(const SMikkTSpaceContext* context, float* out, const int face, const int corner) {
        const Vec3 normal = generationMesh(context).mesh->normals[cornerVertex(context, face, corner)];
        out[0] = normal.x;
        out[1] = normal.y;
        out[2] = normal.z;
    }

    static void cornerUv(const SMikkTSpaceContext* context, float* out, const int face, const int corner) {
        const Vec2 uv = generationMesh(context).mesh->uv0[cornerVertex(context, face, corner)];
        out[0] = uv.x;
        out[1] = uv.y;
    }

    static void storeCornerTangent(const SMikkTSpaceContext* context, const float* tangent, const float sign, const int face, const int corner) {
        TangentGenerationMesh& generation = *static_cast<TangentGenerationMesh*>(context->m_pUserData);
        generation.cornerTangents[cornerIndex(face, corner)] = Vec4(tangent[0], tangent[1], tangent[2], sign);
    }

    MeshImporter::MeshImporter(const ImportSettings& settings, const bool gltfSource) : m_settings(settings), m_gltfSource(gltfSource) {
    }

    bool MeshImporter::read(const aiMesh& source, Mesh& out) const {
        return read(source, Mat4(1.0f), out);
    }

    bool MeshImporter::read(const aiMesh& source, const Mat4& world, Mesh& out) const {
        const bool trianglesOnly = (source.mPrimitiveTypes & aiPrimitiveType_TRIANGLE) != 0 && (source.mPrimitiveTypes & NOT_TRIANGLES) == 0;
        if (!trianglesOnly || source.mNumVertices == 0 || source.mNumFaces == 0) {
            return false;
        }

        const Mat3 linear = Mat3(world);
        const bool mirrored = glm::determinant(linear) < 0.0f;
        std::vector<std::uint32_t> indices;
        indices.reserve(static_cast<std::size_t>(source.mNumFaces) * 3);
        for (unsigned int i = 0; i < source.mNumFaces; i++) {
            const aiFace& face = source.mFaces[i];
            if (face.mNumIndices != 3) {
                return false;
            }
            for (unsigned int corner = 0; corner < 3; corner++) {
                const unsigned int sourceCorner = mirrored && corner > 0 ? 3 - corner : corner;
                if (face.mIndices[sourceCorner] >= source.mNumVertices) {
                    return false;
                }
                indices.push_back(face.mIndices[sourceCorner]);
            }
        }

        std::vector<Vec3> positions;
        positions.reserve(source.mNumVertices);
        for (unsigned int i = 0; i < source.mNumVertices; i++) {
            positions.push_back(bakePosition(Vec3(world * Vec4(toVec3(source.mVertices[i]), 1.0f))));
        }

        out.positions = std::move(positions);
        out.indices = std::move(indices);
        readStreams(source, linear, out);
        generateTangents(out);
        return true;
    }

    const ImportSettings& MeshImporter::getSettings() const {
        return m_settings;
    }

    // A Z-up source turns by -90 degrees about X. That is a rotation, so no triangle changes its winding.
    Vec3 MeshImporter::turnUpright(const Vec3 value) const {
        if (m_settings.upAxis == UpAxis::Z) {
            return Vec3(value.x, value.z, -value.y);
        }
        return value;
    }

    Vec3 MeshImporter::bakePosition(const Vec3 position) const {
        return turnUpright(position) * m_settings.scale;
    }

    // Never scaled: the scale is above zero and the same on every axis, so it changes no direction.
    Vec3 MeshImporter::bakeDirection(const Vec3 direction) const {
        const Vec3 turned = turnUpright(direction);
        const float lengthSquared = glm::dot(turned, turned);
        if (lengthSquared < MIN_DIRECTION_LENGTH_SQUARED) {
            return turned;
        }
        return turned / std::sqrt(lengthSquared);
    }

    void MeshImporter::readStreams(const aiMesh& source, const Mat3& world, Mesh& out) const {
        const std::size_t vertexCount = source.mNumVertices;
        if (source.HasNormals()) {
            const Mat3 normalWorld = normalMatrix(world);
            out.normals.reserve(vertexCount);
            for (std::size_t i = 0; i < vertexCount; i++) {
                out.normals.push_back(bakeDirection(normalWorld * toVec3(source.mNormals[i])));
            }
        }

        if (source.HasTextureCoords(0)) {
            out.uv0.reserve(vertexCount);
            for (std::size_t i = 0; i < vertexCount; i++) {
                out.uv0.push_back(Vec2(source.mTextureCoords[0][i].x, source.mTextureCoords[0][i].y));
            }
        }

        if (source.HasNormals() && source.HasTangentsAndBitangents()) {
            // assimp flips glTF's V but builds the bitangent from glTF's own w, so that bitangent points against the
            // stored UVs. The settings' bake is a rotation, so only a mirror in world flips a sign taken before it.
            const float sourceFlip = m_gltfSource ? -1.0f : 1.0f;
            const float mirrorFlip = glm::determinant(world) < 0.0f ? -1.0f : 1.0f;
            out.tangents.reserve(vertexCount);
            for (std::size_t i = 0; i < vertexCount; i++) {
                const Vec3 normal = toVec3(source.mNormals[i]);
                const Vec3 tangent = toVec3(source.mTangents[i]);
                const Vec3 bitangent = toVec3(source.mBitangents[i]);
                const float handedness = glm::dot(glm::cross(normal, tangent), bitangent) < 0.0f ? -1.0f : 1.0f;
                out.tangents.push_back(Vec4(bakeDirection(world * tangent), handedness * sourceFlip * mirrorFlip));
            }
        }
    }

    // The cofactor matrix is the inverse transpose times the determinant, so it also holds for a matrix that flattens an
    // axis. Its sign is put back, because a mirror's cofactor would turn every normal inward.
    Mat3 MeshImporter::normalMatrix(const Mat3& world) {
        const Mat3 cofactor(glm::cross(world[1], world[2]), glm::cross(world[2], world[0]), glm::cross(world[0], world[1]));
        if (glm::determinant(world) < 0.0f) {
            return -cofactor;
        }
        return cofactor;
    }

    void MeshImporter::generateTangents(Mesh& mesh) {
        if (!mesh.tangents.empty() || mesh.normals.empty() || mesh.uv0.empty()) {
            return;
        }

        TangentGenerationMesh generation;
        generation.mesh = &mesh;
        generation.cornerTangents.resize(mesh.indices.size());

        SMikkTSpaceInterface callbacks{};
        callbacks.m_getNumFaces = faceCount;
        callbacks.m_getNumVerticesOfFace = cornersOfFace;
        callbacks.m_getPosition = cornerPosition;
        callbacks.m_getNormal = cornerNormal;
        callbacks.m_getTexCoord = cornerUv;
        callbacks.m_setTSpaceBasic = storeCornerTangent;

        SMikkTSpaceContext context{};
        context.m_pInterface = &callbacks;
        context.m_pUserData = &generation;
        const tbool generated = genTangSpaceDefault(&context);
        if (generated == 0) {
            TE_LOGGER_WARN("Import: MikkTSpace could not make tangents for a mesh of {0} triangles, so it has none", mesh.indices.size() / 3);
            return;
        }

        // MikkTSpace returns one tangent per face corner, so the corners are welded again into shared vertices.
        reindexByCorner(mesh, generation.cornerTangents);
    }

    // Two corners become one vertex only when all four streams match bit for bit.
    void MeshImporter::reindexByCorner(Mesh& mesh, const std::vector<Vec4>& cornerTangents) {
        const std::size_t cornerCount = mesh.indices.size();
        std::vector<Vec3> positions(cornerCount);
        std::vector<Vec3> normals(cornerCount);
        std::vector<Vec2> uv0(cornerCount);
        for (std::size_t i = 0; i < cornerCount; i++) {
            const std::uint32_t vertex = mesh.indices[i];
            positions[i] = mesh.positions[vertex];
            normals[i] = mesh.normals[vertex];
            uv0[i] = mesh.uv0[vertex];
        }

        const std::array<meshopt_Stream, 4> streams{
            meshopt_Stream{positions.data(), sizeof(Vec3), sizeof(Vec3)},
            meshopt_Stream{normals.data(), sizeof(Vec3), sizeof(Vec3)},
            meshopt_Stream{uv0.data(), sizeof(Vec2), sizeof(Vec2)},
            meshopt_Stream{cornerTangents.data(), sizeof(Vec4), sizeof(Vec4)},
        };
        std::vector<unsigned int> remap(cornerCount);
        const std::size_t vertexCount = meshopt_generateVertexRemapMulti(remap.data(), nullptr, cornerCount, cornerCount, streams.data(), streams.size());

        const auto remapStream = [&remap, cornerCount, vertexCount](auto& stream, const auto& corners) {
            stream.resize(vertexCount);
            meshopt_remapVertexBuffer(stream.data(), corners.data(), cornerCount, sizeof(corners[0]), remap.data());
        };
        remapStream(mesh.positions, positions);
        remapStream(mesh.normals, normals);
        remapStream(mesh.uv0, uv0);
        remapStream(mesh.tangents, cornerTangents);
        meshopt_remapIndexBuffer(mesh.indices.data(), nullptr, cornerCount, remap.data());
    }
}
