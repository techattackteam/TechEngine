#pragma once
#include "serialization/StreamReader.hpp"
#include "serialization/StreamWriter.hpp"
#include "TechEngine/core/resources/IResource.hpp"

namespace TechEngine {
    struct CORE_DLL ModelNode {
        glm::vec3 position{0.0f, 0.0f, 0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f, 1.0f, 1.0f};

        std::string name;
        UUID meshUUID;
        UUID sourceMaterialUUID; // Default material that is loaded from the source file
        std::vector<ModelNode> children;

        bool hasMesh() const {
            return !meshUUID.isNull();
        }

        static void serialize(StreamWriter* writer, const ModelNode& node);

        static void deserialize(StreamReader* reader, ModelNode& node);
    };

    class CORE_DLL ModelResource : public IResource {
    public:
        friend class ModelLoader;

        explicit ModelResource(const std::string& name);

        const ModelNode& getRootNode() const {
            return m_rootNode;
        }

        ModelNode& getRootNode() {
            return m_rootNode;
        }

        void setRootNode(ModelNode node) {
            m_rootNode = std::move(node);
        }

        std::vector<UUID> getMeshesUUIDs() const;

        std::vector<UUID> getMaterialUUIDs() const;

        bool reassignMesh(const UUID& oldUUID, const UUID& newUUID);

        bool reassignMaterial(const UUID& oldUUID, const UUID& newUUID);

    private:
        ModelNode m_rootNode;

        ModelResource();

        static void collectMeshNodesRecursive(const ModelNode& node, std::vector<UUID>& out);

        static void collectMaterialNodesRecursive(const ModelNode& node, std::vector<UUID>& out);

        static bool reassignMeshRecursive(ModelNode& node, const UUID& oldUUID, const UUID& newUUID);

        static bool reassignMaterialRecursive(ModelNode& node, const UUID& oldUUID, const UUID& newUUID);

        static void serialize(StreamWriter* writer, const ModelResource& model);

        static std::shared_ptr<ModelResource> deserialize(StreamReader* reader);
    };
}
