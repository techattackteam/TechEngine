#include "SceneManager.hpp"

#include "core/FileSystem.hpp"
#include "core/CoreExportDll.hpp"
#include "core/Logger.hpp"
#include "scene/GameObject.hpp"
#include "components/render/CameraComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/render/MeshRendererComponent.hpp"
#include "components/physics/BoxColliderComponent.hpp"
#include "components/physics/SphereCollider.hpp"
#include "components/physics/CylinderCollider.hpp"
#include "components/physics/RigidBody.hpp"
#include "components/network/NetworkSync.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "material/MaterialManager.hpp"
#include "mesh/ImportedMesh.hpp"

#include <filesystem>
#include <utils/YAMLUtils.hpp>
#include <fstream>

#include "mesh/MeshManager.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v) {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
        return out;
    }

    YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v) {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
        return out;
    }

    YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v) {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
        return out;
    }

    static void serializeGameObject(YAML::Emitter& out, GameObject* gameObject) {
        out << YAML::BeginMap;
        out << YAML::Key << "Name" << YAML::Value << gameObject->getName();
        out << YAML::Key << "Tag" << YAML::Value << gameObject->getTag();
        //TE_LOGGER_TRACE("Serialize game object with Tag = {0}, name = {1}", gameObject->getTag(), gameObject->getName());
        if (gameObject->hasComponent<TransformComponent>()) {
            out << YAML::Key << "TransformComponent";
            out << YAML::BeginMap;
            auto tc = gameObject->getComponent<TransformComponent>();
            out << YAML::Key << "Position" << YAML::Value << tc->getPosition();
            out << YAML::Key << "Orientation" << YAML::Value << tc->getOrientation();
            out << YAML::Key << "Scale" << YAML::Value << tc->getScale();
            out << YAML::EndMap;
        }

        if (gameObject->hasComponent<CameraComponent>()) {
            out << YAML::Key << "CameraComponent";
            out << YAML::BeginMap;
            auto camera = gameObject->getComponent<CameraComponent>();
            out << YAML::Key << "ProjectionType" << YAML::Value << (int)camera->getProjectionType();
            out << YAML::Key << "MainCamera" << YAML::Value << camera->isMainCamera();
            out << YAML::Key << "Fov" << YAML::Value << camera->getFov();
            out << YAML::Key << "Near" << YAML::Value << camera->getNear();
            out << YAML::Key << "Far" << YAML::Value << camera->getFar();
            out << YAML::EndMap;
        }

        if (gameObject->hasComponent<MeshRendererComponent>()) {
            out << YAML::Key << "MeshRendererComponent";
            out << YAML::BeginMap;
            auto meshRendererComponent = gameObject->getComponent<MeshRendererComponent>();
            Material& material = meshRendererComponent->getMaterial();
            out << YAML::Key << "Mesh" << YAML::Value << meshRendererComponent->getMesh().getName();
            if (meshRendererComponent->getMesh().getName() == "ImportedMesh") {
                std::string path = FileSystem::rootPath.string() + R"(\Resources\Models\)" + gameObject->getParent()->getName() + ".mesh";
                if (!std::filesystem::exists(path)) {
                    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
                }
                YAML::Emitter emitter;
                emitter << YAML::BeginMap;
                emitter << YAML::Key << gameObject->getName() << YAML::Value << YAML::BeginSeq;
                emitter << YAML::BeginMap;
                for (Vertex& vertex: meshRendererComponent->getMesh().getVertices()) {
                    emitter << YAML::Key << "Vertex";
                    emitter << YAML::BeginMap;
                    emitter << YAML::Key << "Position" << YAML::Value << vertex.getPosition();
                    emitter << YAML::Key << "Normal" << YAML::Value << vertex.getNormal();
                    emitter << YAML::Key << "TextureCoordinate" << YAML::Value << vertex.getTextureCoordinate();
                    emitter << YAML::Key << "Color" << YAML::Value << vertex.getColor();
                    emitter << YAML::EndMap;
                }
                emitter << "Indices" << YAML::Value << YAML::BeginSeq;
                for (int& index: meshRendererComponent->getMesh().getIndices()) {
                    emitter << index;
                }
                emitter << YAML::EndSeq;
                emitter << YAML::EndMap;
                emitter << YAML::EndSeq;
                emitter << YAML::EndMap;
                std::filesystem::path filepath = path;
                std::ofstream fout;
                fout.open(filepath, std::ios::app);
                fout << emitter.c_str();
                fout.close();
            }
            out << YAML::Key << "Material" << YAML::Value << material.getName();
            out << YAML::EndMap;
        }

        if (gameObject->hasComponent<BoxColliderComponent>()) {
            out << YAML::Key << "BoxColliderComponent";
            out << YAML::BeginMap;
            auto boxColliderComponent = gameObject->getComponent<BoxColliderComponent>();
            out << YAML::Key << "Size" << YAML::Value << boxColliderComponent->getSize();
            out << YAML::Key << "Offset" << YAML::Value << boxColliderComponent->getOffset();
            out << YAML::EndMap;
        }

        if (gameObject->hasComponent<SphereCollider>()) {
            out << YAML::Key << "SphereCollider";
            out << YAML::BeginMap;
            auto sphereColliderComponent = gameObject->getComponent<SphereCollider>();
            out << YAML::Key << "Radius" << YAML::Value << sphereColliderComponent->getRadius();
            out << YAML::Key << "Offset" << YAML::Value << sphereColliderComponent->getOffset();
            out << YAML::EndMap;
        }
        if (gameObject->hasComponent<CylinderCollider>()) {
            out << YAML::Key << "CylinderCollider";
            out << YAML::BeginMap;
            auto cylinderColliderComponent = gameObject->getComponent<CylinderCollider>();
            out << YAML::Key << "Radius" << YAML::Value << cylinderColliderComponent->getRadius();
            out << YAML::Key << "Height" << YAML::Value << cylinderColliderComponent->getHeight();
            out << YAML::Key << "Offset" << YAML::Value << cylinderColliderComponent->getOffset();
            out << YAML::EndMap;
        }

        if (gameObject->hasComponent<RigidBody>()) {
            out << YAML::Key << "RigidBody";
            out << YAML::BeginMap;
            auto rigidBodyComponent = gameObject->getComponent<RigidBody>();
            out << YAML::Key << "Mass" << YAML::Value << rigidBodyComponent->getMass();
            out << YAML::Key << "Density" << YAML::Value << rigidBodyComponent->getDensity();
            out << YAML::EndMap;
        }

        if (gameObject->hasComponent<NetworkSync>()) {
            out << YAML::Key << "NetworkHandlerComponent";
            out << YAML::BeginMap;
            out << YAML::EndMap;
        }

        if (gameObject->hasChildren()) {
            out << YAML::Key << "Children" << YAML::Value << YAML::BeginSeq;
            for (auto& pair: gameObject->getChildren()) {
                serializeGameObject(out, pair.second);
            }
            out << YAML::EndSeq;
        }

        out << YAML::EndMap;
    }

    SceneManager::SceneManager(SystemsRegistry& systemsRegistry,
                               FilePaths& filePaths) : systemsRegistry(systemsRegistry),
                                                       filePaths(filePaths),
                                                       scene(systemsRegistry) {
    }

    void SceneManager::serialize(const std::string& sceneName, const std::string& filepath) {
        //Measure time
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Scene" << YAML::Value << sceneName;
        out << YAML::Key << "GameObjects" << YAML::Value << YAML::BeginSeq;
        for (GameObject* gameObject: scene.getGameObjects()) {
            if (!gameObject->hasParent()) {
                serializeGameObject(out, gameObject);
            }
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;
        std::filesystem::path path = filepath;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream fout(filepath);
        fout << out.c_str();
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - begin;
        //TE_LOGGER_INFO("Scene {0} saved in {1} seconds", sceneName, elapsed_seconds.count());
    }

    void SceneManager::deserializeGameObject(YAML::Node gameObjectYAML, GameObject* parent) {
        auto name = gameObjectYAML["Name"].as<std::string>();
        auto tag = gameObjectYAML["Tag"].as<std::string>();
        GameObject& gameObject = scene.registerGameObject(name, tag);
        if (parent != nullptr) {
            scene.makeChildTo(parent, &gameObject);
        }

        auto transformComponentNode = gameObjectYAML["TransformComponent"];
        if (transformComponentNode) {
            auto tc = gameObject.getComponent<TransformComponent>();
            tc->translateTo(transformComponentNode["Position"].as<glm::vec3>());
            tc->setRotation(transformComponentNode["Orientation"].as<glm::vec3>());
            tc->setScale(transformComponentNode["Scale"].as<glm::vec3>());
        }

        auto ccNode = gameObjectYAML["CameraComponent"];
        if (ccNode) {
            gameObject.addComponent<CameraComponent>();
            CameraComponent* cameraComponent = gameObject.getComponent<CameraComponent>();
            cameraComponent->changeProjectionType((CameraComponent::ProjectionType)ccNode["ProjectionType"].as<int>());
            cameraComponent->setIsMainCamera(ccNode["MainCamera"].as<bool>());
            cameraComponent->setFov(ccNode["Fov"].as<float>());
            cameraComponent->setNear(ccNode["Near"].as<float>());
            cameraComponent->setFar(ccNode["Far"].as<float>());
        }

        auto meshRendererNode = gameObjectYAML["MeshRendererComponent"];
        if (meshRendererNode) {
            std::string meshName = meshRendererNode["Mesh"].as<std::string>();
            std::string materialName = meshRendererNode["Material"].as<std::string>();

            Material& material = systemsRegistry.getSystem<MaterialManager>().getMaterial(materialName);
            /*
            Mesh* mesh;
            if (meshName == "Cube") {
                mesh = new CubeMesh();
            } else if (meshName == "Sphere") {
                mesh = new SphereMesh();
            } else if (meshName == "Cylinder") {
                mesh = new CylinderMesh();
            } else if (meshName == "ImportedMesh") {
                std::string filepath = FileSystem::rootPath.string() + R"(\Resources\Models\)" + gameObject.getParent()->getName() + ".mesh";
                YAML::Node data;
                try {
                    data = YAML::LoadFile(filepath);
                } catch (YAML::Exception& e) {
                    TE_LOGGER_CRITICAL("Failed to load .scene file {0}.\n      {1}", filepath, e.what());
                    return;
                }
                std::vector<Vertex> vertices;
                std::vector<int> indices;
                auto meshNode = data[gameObject.getName()];
                for (auto vertexNode: meshNode["Vertex"]) {
                    Vertex vertex(vertexNode["Position"].as<glm::vec3>(), vertexNode["Normal"].as<glm::vec3>(), vertexNode["TextureCoordinate"].as<glm::vec2>());
                    vertex.setColor(vertexNode["Color"].as<glm::vec4>());
                    vertices.push_back(vertex);
                }
                for (auto indexNode: meshNode["Index"]) {
                    indices.push_back(indexNode.as<int>());
                }
                mesh = new ImportedMesh(vertices, indices);
            } else {
                TE_LOGGER_CRITICAL("Failed to deserialize mesh renderer component.\n      Mesh name {0} is not valid.", meshName);
            }
            */
            Mesh& mesh = systemsRegistry.getSystem<MeshManager>().getMesh(meshName);
            ScriptRegister::getInstance();
            gameObject.addComponent<MeshRendererComponent>(mesh, &material);
        }
        auto boxColliderNode = gameObjectYAML["BoxColliderComponent"];
        if (boxColliderNode) {
            gameObject.addComponent<BoxColliderComponent>();
            BoxColliderComponent* boxColliderComponent = gameObject.getComponent<BoxColliderComponent>();
            boxColliderComponent->setSize(boxColliderNode["Size"].as<glm::vec3>());
            boxColliderComponent->setOffset(boxColliderNode["Offset"].as<glm::vec3>());
        }

        auto sphereColliderNode = gameObjectYAML["SphereCollider"];
        if (sphereColliderNode) {
            gameObject.addComponent<SphereCollider>();
            SphereCollider* sphereColliderComponent = gameObject.getComponent<SphereCollider>();
            sphereColliderComponent->setRadius(sphereColliderNode["Radius"].as<float>());
            sphereColliderComponent->setOffset(sphereColliderNode["Offset"].as<glm::vec3>());
        }

        auto cylinderColliderNode = gameObjectYAML["CylinderCollider"];
        if (cylinderColliderNode) {
            gameObject.addComponent<CylinderCollider>();
            CylinderCollider* cylinderColliderComponent = gameObject.getComponent<CylinderCollider>();
            cylinderColliderComponent->setRadius(cylinderColliderNode["Radius"].as<float>());
            cylinderColliderComponent->setHeight(cylinderColliderNode["Height"].as<float>());
            cylinderColliderComponent->setOffset(cylinderColliderNode["Offset"].as<glm::vec3>());
        }

        auto rigidBodyNode = gameObjectYAML["RigidBody"];
        if (rigidBodyNode) {
            gameObject.addComponent<RigidBody>();
            RigidBody* rigidBodyComponent = gameObject.getComponent<RigidBody>();
            rigidBodyComponent->setMass(rigidBodyNode["Mass"].as<float>());
            rigidBodyComponent->setDensity(rigidBodyNode["Density"].as<float>());
            systemsRegistry.getSystem<PhysicsEngine>().addRigidBody(rigidBodyComponent);
        }
        auto networkHandlerNode = gameObjectYAML["NetworkHandlerComponent"];
        if (networkHandlerNode) {
            gameObject.addComponent<NetworkSync>();
        }
        auto childrenNode = gameObjectYAML["Children"];
        for (auto childNodeYAML: childrenNode) {
            if (childrenNode) {
                deserializeGameObject(childNodeYAML, &gameObject);
            }
        }
    }

    bool SceneManager::deserialize(const std::string& filepath) {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        YAML::Node data;
        try {
            data = YAML::LoadFile(filepath);
        } catch (YAML::Exception& e) {
            TE_LOGGER_CRITICAL("Failed to load .scene file {0}.\n      {1}", filepath, e.what());
        }

        YAML::Node node = data["GameObjects"];
        if (node) {
            for (YAML::Node gameObjectYAML: node) {
                deserializeGameObject(gameObjectYAML, nullptr);
            }
        }

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - begin;
        //TE_LOGGER_INFO("Scene {0} loaded in {1} seconds", getSceneNameFromPath(filepath), elapsed_seconds.count());
        return true;
    }

    void SceneManager::init(const std::vector<std::string>& scenes) {
        for (std::string scene: scenes) {
            registerScene(scene);
        }
    }

    void SceneManager::registerScene(std::string& scenePath) {
        std::string sceneName = getSceneNameFromPath(scenePath);
        if (m_scenesBank.find(sceneName) != m_scenesBank.end()) {
            replaceSceneNameFromPath(scenePath);
            sceneName = getSceneNameFromPath(scenePath);
            m_scenesBank[sceneName] = scenePath;
        } else {
            m_scenesBank[sceneName] = scenePath;
        }
    }

    void SceneManager::createNewScene(std::string& scenePath) {
        std::string sceneName = getSceneNameFromPath(scenePath);
        registerScene(scenePath);
        std::filesystem::copy(FileSystem::scenesTemplate, scenePath);
    }

    bool SceneManager::deleteScene(std::string& scenePath) {
        std::string sceneName = getSceneNameFromPath(scenePath);
        if (m_activeSceneName == sceneName) {
            TE_LOGGER_CRITICAL("Failed to delete scene '{0}'.\n Cannot delete active scene.", sceneName);
            return false;
        } else if (hasScene(sceneName)) {
            std::filesystem::remove(scenePath);
            m_scenesBank.erase(sceneName);
            return true;
        } else {
            TE_LOGGER_CRITICAL("Failed to delete scene '{0}'.\n Scene not found!", sceneName);
            return false;
        }
    }

    bool SceneManager::hasScene(const std::string& sceneName) {
        return m_scenesBank.find(sceneName) != m_scenesBank.end();
    }

    void SceneManager::loadScene(const std::string& sceneName) {
        if (hasScene(sceneName)) {
            if (!m_activeSceneName.empty()) {
                if (m_activeSceneName != sceneName) {
                    saveScene(m_activeSceneName);
                }
                scene.clear();
                systemsRegistry.getSystem<PhysicsEngine>().clear();
            }
            std::string scenePath = m_scenesBank[sceneName];
            deserialize(scenePath);
            m_activeSceneName = sceneName;
        } else {
            TE_LOGGER_ERROR("Failed to load scene '{0}'.\n Could not find scene. Creating default one", sceneName);
            scene.clear();
            systemsRegistry.getSystem<PhysicsEngine>().clear();
            m_activeSceneName = "DefaultScene";
            GameObject& gameObject = scene.createGameObject("Main Camera");
            gameObject.addComponent<CameraComponent>();
            gameObject.getComponent<CameraComponent>()->setIsMainCamera(true);
            gameObject.getComponent<TransformComponent>()->translateTo(glm::vec3(0.0f, 0.0f, 5.0f));
            GameObject& cube = scene.createGameObject("Cube");
            cube.addComponent<MeshRendererComponent>(systemsRegistry.getSystem<MeshManager>().getMesh("Cube"), &systemsRegistry.getSystem<MaterialManager>().getMaterial("DefaultMaterial"));
            cube.getComponent<TransformComponent>()->translateTo(glm::vec3(0.0f, 0.0f, 0.0f));
            std::string scenePath = filePaths.commonAssetsPath + "\\Scenes\\" + m_activeSceneName + ".scene";
            registerScene(scenePath);
            saveScene(m_activeSceneName);
        }
    }

    void SceneManager::saveScene(const std::string& sceneName) {
        if (m_scenesBank.find(sceneName) != m_scenesBank.end()) {
            std::string scenePath = m_scenesBank[sceneName];
            serialize(sceneName, scenePath);
        } else {
            TE_LOGGER_ERROR("Failed to save scene '{0}'", sceneName);
        }
    }

    void SceneManager::saveCurrentScene() {
        saveScene(m_activeSceneName);
    }

    void SceneManager::saveSceneAsTemporarily(const std::string& cachPath, CompileProject compileProject) {
        std::string sceneName = compileProject == CompileProject::PROJECT_CLIENT ? "SceneClientTemporary" : "SceneServerTemporary";
        std::string sceneTemporaryPath = cachPath + "\\" + sceneName + ".scene";
        serialize(sceneName, sceneTemporaryPath);
    }

    void SceneManager::loadSceneFromTemporarily(const std::string& cachPath, CompileProject compileProject) {
        scene.clear();
        systemsRegistry.getSystem<PhysicsEngine>().clear();
        std::string sceneName = compileProject == CompileProject::PROJECT_CLIENT ? "SceneClientTemporary" : "SceneServerTemporary";
        std::string sceneTemporaryPath = cachPath + "\\" + sceneName + ".scene";
        deserialize(sceneTemporaryPath);
        std::filesystem::remove(sceneTemporaryPath);
    }

    const std::string& SceneManager::getActiveSceneName() {
        return m_activeSceneName;
    }

    void SceneManager::replaceSceneNameFromPath(std::string& scenePath) {
        std::string sceneName = getSceneNameFromPath(scenePath);
        int i = 1;
        while (m_scenesBank.find(sceneName + std::to_string(i)) != m_scenesBank.end()) {
            i++;
        }
        scenePath = scenePath.substr(0, scenePath.find_last_of("\\/")) + "\\" + sceneName + std::to_string(i) + ".scene";
    }

    std::string SceneManager::getSceneNameFromPath(const std::string& scenePath) {
        return scenePath.substr(scenePath.find_last_of("\\/") + 1, scenePath.find_last_of('.') - scenePath.find_last_of("\\/") - 1);
    }

    Scene& SceneManager::getScene() {
        return scene;
    }
}
