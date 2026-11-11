#pragma once

#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/assets/ImportBlock.hpp>

#include <vector>

struct aiMesh;

namespace TechEngine {
    struct Mesh;

    class MeshImporter {
    private:
        ImportSettings m_settings;
        bool m_gltfSource = false;

    public:
        MeshImporter(const ImportSettings& settings, bool gltfSource);

        // Fills the positions, indices and vertex streams, baked by the settings, and leaves the rest of out alone.
        // Returns false for a mesh that is not made of triangles.
        bool read(const aiMesh& source, Mesh& out) const;

        const ImportSettings& getSettings() const;

        Vec3 turnUpright(Vec3 value) const;

        Vec3 bakePosition(Vec3 position) const;

    private:
        Vec3 bakeDirection(Vec3 direction) const;

        void readStreams(const aiMesh& source, Mesh& out) const;

        static void generateTangents(Mesh& mesh);

        static void reindexByCorner(Mesh& mesh, const std::vector<Vec4>& cornerTangents);
    };
}
