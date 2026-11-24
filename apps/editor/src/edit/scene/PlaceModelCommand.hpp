#pragma once

#include <TechEngine/app/Command.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace TechEngine {
    class AssetIndex;
    class EditSession;
    struct AssetRecord;
    class FileAccess;
    class ResourceCache;
    struct Model;
    struct SceneBytes;

    class PlaceModelCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        const FileAccess* m_files = nullptr;
        ResourceCache* m_resources = nullptr;
        const ResourceHandle<SceneBytes>* m_sceneHandle = nullptr;
        const AssetIndex* m_assets = nullptr;
        std::string m_path;

    public:
        PlaceModelCommand(EditSession& session, const FileAccess& files, ResourceCache& resources, const ResourceHandle<SceneBytes>& sceneHandle, const AssetIndex& assets, std::string path);

        void apply(Scene& scene) override;

    private:
        bool readModel(Model& out) const;

        static void wrapMesh(const AssetRecord& record, Model& out);

        bool loadMeshes(const Model& model, std::unordered_map<ResourceId, ResourceSlotId>& out) const;

        bool buildEntities(Scene& scene, const Model& model, const std::unordered_map<ResourceId, ResourceSlotId>& meshes, std::vector<Entity>& out) const;

        void removeEntities(Scene& scene, std::vector<Entity>& entities) const;
    };
}
