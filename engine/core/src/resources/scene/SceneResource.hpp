#pragma once

#include "physics/PhysicsEngine.hpp"
#include "TechEngine/core/resources/IResource.hpp"

#include <filesystem>

namespace TechEngine {
    class Scene;

    class CORE_DLL SceneResource : public IResource {
    private:
        std::filesystem::path m_virtualPath;

    public:
        SceneResource(const std::string& name, const std::filesystem::path& path);

        const std::filesystem::path& getVirtualPath() const;

        void setPath(const std::filesystem::path& path);

        static void serialize(StreamWriter* writer, const SceneResource& resource, const Scene& scene);

        static std::shared_ptr<SceneResource> deserializeMetadata(StreamReader* reader);

        static std::shared_ptr<SceneResource> deserialize(StreamReader* reader, Scene& scene, PhysicsEngine& physicsEngine);
    };
}
