#include "TechEngine/core/resources/model/ModelResource.hpp"

namespace TechEngine {
    void ModelNode::serialize(StreamWriter* writer, const ModelNode& node) {
        writer->writeString(node.name);
        writer->writeRaw(node.position);
        writer->writeRaw(node.rotation);
        writer->writeRaw(node.scale);
        writer->writeRaw(node.meshUUID);
        writer->writeRaw(node.sourceMaterialUUID);
        const auto childCount = static_cast<uint32_t>(node.children.size());
        writer->writeRaw(childCount);
        for (const ModelNode& child: node.children) {
            serialize(writer, child);
        }
    }

    void ModelNode::deserialize(StreamReader* reader, ModelNode& node) {
        reader->readString(node.name);
        reader->readRaw(node.position);
        reader->readRaw(node.rotation);
        reader->readRaw(node.scale);
        reader->readRaw(node.meshUUID);
        reader->readRaw(node.sourceMaterialUUID);
        uint32_t childCount = 0;
        reader->readRaw(childCount);
        node.children.resize(childCount);
        for (ModelNode& child: node.children) {
            deserialize(reader, child);
        }
    }

    ModelResource::ModelResource(const std::string& name) : IResource(name) {
    }

    std::vector<UUID> ModelResource::getMeshesUUIDs() const {
        std::vector<UUID> nodes;
        collectMeshNodesRecursive(m_rootNode, nodes);
        return nodes;
    }

    std::vector<UUID> ModelResource::getMaterialUUIDs() const {
        std::vector<UUID> materials;
        collectMaterialNodesRecursive(m_rootNode, materials);
        return materials;
    }

    bool ModelResource::reassignMesh(const UUID& oldUUID, const UUID& newUUID) {
        return reassignMeshRecursive(m_rootNode, oldUUID, newUUID);
    }

    bool ModelResource::reassignMaterial(const UUID& oldUUID, const UUID& newUUID) {
        return reassignMaterialRecursive(m_rootNode, oldUUID, newUUID);
    }

    ModelResource::ModelResource() : IResource() {
    }

    void ModelResource::collectMeshNodesRecursive(const ModelNode& node, std::vector<UUID>& out) {
        if (node.hasMesh()) {
            out.push_back(node.meshUUID);
        }
        for (const ModelNode& child: node.children) {
            collectMeshNodesRecursive(child, out);
        }
    }

    void ModelResource::collectMaterialNodesRecursive(const ModelNode& node, std::vector<UUID>& out) {
        if (!node.sourceMaterialUUID.isNull()) {
            out.push_back(node.sourceMaterialUUID);
        }
        for (const ModelNode& child: node.children) {
            collectMaterialNodesRecursive(child, out);
        }
    }

    bool ModelResource::reassignMeshRecursive(ModelNode& node, const UUID& oldUUID, const UUID& newUUID) {
        bool changed = false;
        if (node.meshUUID == oldUUID) {
            node.meshUUID = newUUID;
            changed = true;
        }
        for (ModelNode& child: node.children) {
            changed |= reassignMeshRecursive(child, oldUUID, newUUID);
        }
        return changed;
    }

    bool ModelResource::reassignMaterialRecursive(ModelNode& node, const UUID& oldUUID, const UUID& newUUID) {
        bool changed = false;
        if (node.sourceMaterialUUID == oldUUID) {
            node.sourceMaterialUUID = newUUID;
            changed = true;
        }
        for (ModelNode& child: node.children) {
            changed |= reassignMaterialRecursive(child, oldUUID, newUUID);
        }
        return changed;
    }

    void ModelResource::serialize(StreamWriter* writer, const ModelResource& model) {
        IResource::serialize(writer, model);
        writer->writeString(model.m_name);
        ModelNode::serialize(writer, model.m_rootNode);
    }

    std::shared_ptr<ModelResource> ModelResource::deserialize(StreamReader* reader) {
        std::shared_ptr<ModelResource> modelResource = std::shared_ptr<ModelResource>(new ModelResource());
        IResource::deserialize(reader, *modelResource);
        reader->readString(modelResource->m_name);
        ModelNode::deserialize(reader, modelResource->m_rootNode);
        return modelResource;
    }
}
