#pragma once
#include "AssimpLoader.hpp"
#include "resources/IResourceLoader.hpp"
#include "resources/ResourceSystem.hpp"
#include <unordered_map>

namespace TechEngine {
    class MeshLoader : public IResourceLoader {
    public:
        enum AtlasType : int {
            ATLAS_ALBEDO = 0, ATLAS_NORMAL, ATLAS_METALLIC,
            ATLAS_ROUGHNESS, ATLAS_AO, ATLAS_EMISSION, ATLAS_TYPE_COUNT
        };

        struct Chart {
            std::vector<int> faces;
            std::vector<glm::vec2> uvs; // parameterized atlas UVs
            std::unordered_map<int, int> globalToLocal;
            std::vector<int> localToGlobal;
            glm::vec2 bbMin{}, bbMax{};
            int pixW = 0, pixH = 0;
            int packX = 0, packY = 0;
            float texelDensity = 256.0f;
            int materialIdx = 0;
            glm::ivec2 uvOffset{0, 0}; // NEW: integer tile cell
        };

    private:
        FileSystem& m_fileSystem;
        AssimpLoader m_assimpLoader;
        TextureLoader& m_textureLoader;
        MaterialLoader& m_materialLoader;
        ResourceSystem& m_resourcesSystem;

        static constexpr float CHART_ANGLE_THRESHOLD = 66.0f;
        static constexpr int ATLAS_PADDING = 2;
        static constexpr int RELAX_ITERATIONS = 40;
        static constexpr int ATLAS_CHANNELS = 4;
        static constexpr int MAX_ATLAS_SIZE = 8192;

        struct FlatSubMesh {
            const AssimpLoader::aiMeshData* data;
            glm::mat4 worldTransform;
        };

        static void flattenNodeTree(const AssimpLoader::Node& node,
                                    std::vector<FlatSubMesh>& out, const glm::mat4& parentWorld = glm::mat4(1.0f));

        std::vector<Chart> generateCharts(
            const std::vector<Vertex>& verts, const std::vector<int>& indices,
            const std::vector<int>& faceMats, float angleDeg) const;

        static void parameterizeChart(Chart& chart,
                                      const std::vector<Vertex>& verts, const std::vector<int>& indices);

        static void relaxChart(Chart& chart,
                               const std::vector<Vertex>& verts, const std::vector<int>& indices, int iters);

        void computeChartSizes(std::vector<Chart>& charts,
                               const std::vector<Vertex>& verts, const std::vector<int>& indices,
                               const std::vector<int>& faceMats, const std::vector<std::string>& matOrder,
                               const AssimpLoader::SceneData& scene) const;

        static bool packCharts(std::vector<Chart>& charts,
                               int maxSize, int padding, int& outW, int& outH);

        void rasterizeAtlas(
            std::vector<unsigned char>& pixels, int atlasW, int atlasH,
            const std::vector<Chart>& charts,
            const std::vector<Vertex>& verts, const std::vector<int>& indices,
            const std::vector<std::string>& matOrder,
            const AssimpLoader::SceneData& scene, AtlasType type) const;

        std::shared_ptr<TextureResource> findTexture(
            const std::unordered_map<std::string, std::shared_ptr<TextureResource>>& textures,
            const UUID& uuid) const;

        static UUID getTexUUID(const MaterialResource& mat, AtlasType type);

        static const char* atlasTypeName(AtlasType type);

    public:
        MeshLoader(ResourceSystem& resourceSystem, FileSystem& fileSystem, TextureLoader& textureLoader, MaterialLoader& materialLoader);

        ~MeshLoader() override = default;

        void importMeshFile(const std::filesystem::path& virtualPath);

        std::tuple<std::shared_ptr<MeshResource>,
            std::shared_ptr<MaterialResource>,
            std::vector<std::shared_ptr<TextureResource>>> loadMeshFromSourceFile(const std::filesystem::path& virtualPath);

        void serializeMeshResource(Buffer& buffer, const std::string& meshName);

        void serializeMeshResource(Buffer& buffer, const std::shared_ptr<MeshResource>& mesh);

        std::shared_ptr<MeshResource> deserializeMeshResource(Buffer& buffer) const;

        std::vector<std::string> sourceExtensions() const override;

        std::string resourceExtension() const override;
    };
}
