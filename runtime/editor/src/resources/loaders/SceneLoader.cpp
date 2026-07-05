#include "SceneLoader.hpp"

#include "fileSystem/FileSystem.hpp"
#include "systems/SystemsRegistry.hpp"
#include "resources/ResourceSystem.hpp"
#include "scene/SceneInternal.hpp"
#include "scene/SceneManager.hpp"
#include "serialization/BufferStream.hpp"
#include "physics/PhysicsEngine.hpp"
#include "eventSystem/EventManager.hpp"
#include "TechEngine/core/events/scene/SceneLoadEvent.hpp"

namespace TechEngine {
    SceneLoader::SceneLoader(SystemsRegistry& systemsRegistry, FileSystem& fileSystem) : m_systemsRegistry(systemsRegistry),
                                                                                         m_fileSystem(fileSystem) {
    }

    SceneLoader::~SceneLoader() {
    }

    bool SceneLoader::createScene(const std::string& name, const std::filesystem::path& virtualPath) {
        ResourceSystem& resourceSystem = m_systemsRegistry.getSystem<ResourceSystem>();

        if (resourceSystem.isSceneRegistered(name)) {
            TE_LOGGER_WARN("Scene already exists: {0}", name);
            return false;
        }

        Scene scene = Scene(m_systemsRegistry);
        scene.getInternal()->setName(name);

        auto resource = std::make_shared<SceneResource>(name, virtualPath);
        if (!resourceSystem.registerSceneResource(resource)) {
            return false;
        }

        Buffer buffer;
        BufferStreamWriter writer = BufferStreamWriter(buffer, 0);
        SceneResource::serialize(&writer, *resource, scene);
        m_fileSystem.write(virtualPath, buffer);
        return true;
    }

    bool SceneLoader::saveScene(const std::filesystem::path& virtualPath) {
        FileStatus status = m_fileSystem.status(virtualPath);
        if (!status.exists) {
            TE_LOGGER_ERROR("Cannot save scene, file does not exist: {0}", virtualPath.string());
            return false;
        }
        ResourceSystem& resourceSystem = m_systemsRegistry.getSystem<ResourceSystem>();
        std::shared_ptr<SceneResource> resource = resourceSystem.getSceneResource(status.name.string());
        if (!resource) {
            TE_LOGGER_ERROR("Cannot save scene, resource not found: {0}", virtualPath.string());
            return false;
        }

        Buffer buffer;
        BufferStreamWriter writer = BufferStreamWriter(buffer, 0);
        SceneManager& sceneManager = m_systemsRegistry.getSystem<SceneManager>();
        SceneResource::serialize(&writer, *resource, sceneManager.getActiveScene());
        m_fileSystem.write(resource->getVirtualPath(), buffer);
        return true;
    }

    bool SceneLoader::saveActiveScene() {
        SceneManager& sceneManager = m_systemsRegistry.getSystem<SceneManager>();
        ResourceSystem& resourceSystem = m_systemsRegistry.getSystem<ResourceSystem>();
        std::shared_ptr<SceneResource> resource = resourceSystem.getSceneResource(sceneManager.getActiveSceneName());
        if (!resource) {
            TE_LOGGER_ERROR("Cannot save active scene, resource not found: {0}", sceneManager.getActiveSceneName());
            return false;
        }
        return saveScene(resource->getVirtualPath());
    }

    bool SceneLoader::loadScene(const std::filesystem::path& virtualPath) {
        FileStatus status = m_fileSystem.status(virtualPath);
        if (!status.exists) {
            TE_LOGGER_ERROR("Cannot load scene, file does not exist: {0}", virtualPath.string());
            return false;
        }

        ResourceSystem& resourceSystem = m_systemsRegistry.getSystem<ResourceSystem>();
        std::shared_ptr<SceneResource> resource = resourceSystem.getSceneResource(status.name.string());
        if (!resource) {
            TE_LOGGER_ERROR("Cannot load scene, resource not found: {0}", status.name.string());
            return false;
        }

        Buffer buffer;
        if (!m_fileSystem.read(resource->getVirtualPath(), buffer)) {
            TE_LOGGER_ERROR("Cannot read scene file: {0}", resource->getVirtualPath().string());
            return false;
        }

        SceneManager& sceneManager = m_systemsRegistry.getSystem<SceneManager>();
        sceneManager.clear();

        BufferStreamReader reader = BufferStreamReader(buffer, 0);
        Scene& scene = sceneManager.getActiveScene();
        PhysicsEngine& physicsEngine = m_systemsRegistry.getSystem<PhysicsEngine>();
        SceneResource::deserialize(&reader, scene, physicsEngine);

        m_systemsRegistry.getSystem<EventManager>().dispatch<SceneLoadEvent>(scene.getName());

        return true;
    }

    bool SceneLoader::loadScene(const std::string& name) {
        ResourceSystem& resourceSystem = m_systemsRegistry.getSystem<ResourceSystem>();
        std::shared_ptr<SceneResource> resource = resourceSystem.getSceneResource(name);
        if (!resource) {
            TE_LOGGER_ERROR("Cannot load scene, resource not found: {0}", name);
            return false;
        }
        return loadScene(resource->getVirtualPath());
    }

    bool SceneLoader::deleteScene(const std::filesystem::path& virtualPath) const {
        FileStatus status = m_fileSystem.status(virtualPath);
        if (!status.exists) {
            TE_LOGGER_ERROR("Cannot delete scene, file does not exist: {0}", virtualPath.string());
            return false;
        }

        ResourceSystem& resourceSystem = m_systemsRegistry.getSystem<ResourceSystem>();
        std::shared_ptr<SceneResource> resource = resourceSystem.getSceneResource(status.name.string());
        if (!resource) {
            TE_LOGGER_ERROR("Cannot delete scene, resource not found: {0}", status.name.string());
            return false;
        }

        m_fileSystem.deleteFile(resource->getVirtualPath());
        resourceSystem.unregisterSceneResource(status.name.string());
        return true;
    }

    bool SceneLoader::loadSceneMetadata(Buffer& buffer) {
        BufferStreamReader reader = BufferStreamReader(buffer, 0);
        auto resource = SceneResource::deserializeMetadata(&reader);
        if (!resource) {
            return false;
        }
        return m_systemsRegistry.getSystem<ResourceSystem>().registerSceneResource(resource);
    }

    std::vector<std::string> SceneLoader::sourceExtensions() const {
        return {};
    }

    std::string SceneLoader::resourceExtension() const {
        return ".tescene";
    }
}
