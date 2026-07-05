#pragma once
#include <filesystem>

#include "fileSystem/FileSystem.hpp"
#include "resources/IResourceLoader.hpp"
#include "resources/scene/SceneResource.hpp"

namespace TechEngine {
    class Scene;
    class PhysicsEngine;

    class SceneLoader : public IResourceLoader {
    private:
        SystemsRegistry& m_systemsRegistry;
        FileSystem& m_fileSystem;

    public:
        SceneLoader(SystemsRegistry& systemsRegistry, FileSystem& fileSystem);

        ~SceneLoader() override;

        bool createScene(const std::string& name, const std::filesystem::path& virtualPath);

        bool saveScene(const std::filesystem::path& virtualPath);

        bool saveActiveScene();

        bool loadScene(const std::filesystem::path& virtualPath);

        bool loadScene(const std::string& name);

        bool deleteScene(const std::filesystem::path& virtualPath) const;

        bool loadSceneMetadata(Buffer& buffer);

        std::vector<std::string> sourceExtensions() const override;

        std::string resourceExtension() const override;
    };
}
