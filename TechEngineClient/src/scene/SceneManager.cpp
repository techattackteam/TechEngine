#include "SceneManager.hpp"

#include "core/ConstantPaths.hpp"
#include "scene/GameObject.hpp"
#include "components/CameraComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/MeshRendererComponent.hpp"
#include "components/physics/BoxColliderComponent.hpp"
#include "components/physics/SphereCollider.hpp"
#include "core/Logger.hpp"
#include "SceneHelper.hpp"
#include "mesh/CubeMesh.hpp"
#include "mesh/SphereMesh.hpp"
#include "mesh/CylinderMesh.hpp"
#include "mesh/CapsuleMesh.hpp"
#include "components/physics/CylinderCollider.hpp"
#include <filesystem>
#include <yaml-cpp/yaml.h>
#include <fstream>

namespace YAML {

    template<>
    struct convert<glm::vec2> {
        static Node encode(const glm::vec2 &rhs) {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node &node, glm::vec2 &rhs) {
            if (!node.IsSequence() || node.size() != 2)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            return true;
        }
    };

    template<>
    struct convert<glm::vec3> {
        static Node encode(const glm::vec3 &rhs) {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node &node, glm::vec3 &rhs) {
            if (!node.IsSequence() || node.size() != 3)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            return true;
        }
    };

    template<>
    struct convert<glm::vec4> {
        static Node encode(const glm::vec4 &rhs) {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.push_back(rhs.w);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node &node, glm::vec4 &rhs) {
            if (!node.IsSequence() || node.size() != 4)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            rhs.w = node[3].as<float>();
            return true;
        }
    };

}
namespace TechEngine {
    YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec2 &v) {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
        return out;
    }

    YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec3 &v) {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
        return out;
    }

    YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec4 &v) {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
        return out;
    }

    static void serializeGameObject(YAML::Emitter &out, GameObject *gameObject) {
        out << YAML::BeginMap;
        out << YAML::Key << "Name" << YAML::Value << gameObject->getName();
        out << YAML::Key << "Tag" << YAML::Value << gameObject->getTag();
        TE_LOGGER_TRACE("Serialize game object with Tag = {0}, name = {1}", gameObject->getTag(), gameObject->getName());
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
            out << YAML::Key << "ProjectionType" << YAML::Value << (int) camera->getProjectionType();
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
            Material &material = meshRendererComponent->getMaterial();
            out << YAML::Key << "Mesh" << YAML::Value << meshRendererComponent->getMesh().getName();
            out << YAML::Key << "Color" << YAML::Value << material.getColor();
            out << YAML::Key << "Ambient" << YAML::Value << material.getAmbient();
            out << YAML::Key << "Diffuse" << YAML::Value << material.getDiffuse();
            out << YAML::Key << "Specular" << YAML::Value << material.getSpecular();
            out << YAML::Key << "Shininess" << YAML::Value << material.getShininess();
            out << YAML::EndMap;
        }

        if (gameObject->hasComponent<BoxColliderComponent>()) {
            out << YAML::Key << "BoxColliderComponent";
            out << YAML::BeginMap;
            auto boxColliderComponent = gameObject->getComponent<BoxColliderComponent>();
            out << YAML::Key << "Size" << YAML::Value << boxColliderComponent->getSize();
            out << YAML::Key << "Offset" << YAML::Value << boxColliderComponent->getOffset();
            out << YAML::Key << "IsDynamic" << YAML::Value << boxColliderComponent->isDynamic();
            out << YAML::EndMap;
        }

        if (gameObject->hasComponent<SphereCollider>()) {
            out << YAML::Key << "SphereCollider";
            out << YAML::BeginMap;
            auto sphereColliderComponent = gameObject->getComponent<SphereCollider>();
            out << YAML::Key << "Radius" << YAML::Value << sphereColliderComponent->getRadius();
            out << YAML::Key << "Offset" << YAML::Value << sphereColliderComponent->getOffset();
            out << YAML::Key << "IsDynamic" << YAML::Value << sphereColliderComponent->isDynamic();
            out << YAML::EndMap;
        }
        if (gameObject->hasComponent<CylinderCollider>()) {
            out << YAML::Key << "CylinderCollider";
            out << YAML::BeginMap;
            auto cylinderColliderComponent = gameObject->getComponent<CylinderCollider>();
            out << YAML::Key << "Radius" << YAML::Value << cylinderColliderComponent->getRadius();
            out << YAML::Key << "Height" << YAML::Value << cylinderColliderComponent->getHeight();
            out << YAML::Key << "Offset" << YAML::Value << cylinderColliderComponent->getOffset();
            out << YAML::Key << "IsDynamic" << YAML::Value << cylinderColliderComponent->isDynamic();
            out << YAML::EndMap;
        }

        if (gameObject->hasChildren()) {
            out << YAML::Key << "Children" << YAML::Value << YAML::BeginSeq;
            for (auto &pair: gameObject->getChildren()) {
                serializeGameObject(out, pair.second);
            }
            out << YAML::EndSeq;
        }

        out << YAML::EndMap;
    }

    void SceneManager::serialize(const std::string &sceneName, const std::string &filepath) {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Scene" << YAML::Value << sceneName;
        out << YAML::Key << "GameObjects" << YAML::Value << YAML::BeginSeq;
        for (GameObject *gameObject: Scene::getInstance().getGameObjects()) {
            serializeGameObject(out, gameObject);
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;
        std::filesystem::path path = filepath;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    static void deserializeGameObject(YAML::Node gameObjectYAML, GameObject *parent) {
        auto name = gameObjectYAML["Name"].as<std::string>();
        auto tag = gameObjectYAML["Tag"].as<std::string>();
        GameObject *gameObject = new GameObject(name, tag);
        if (parent != nullptr) {
            Scene::getInstance().makeChildTo(parent, gameObject);
        }
        TE_LOGGER_TRACE("Deserialized game object with Tag = {0}, name = {1}", gameObject->getTag(), name);

        auto transformComponentNode = gameObjectYAML["TransformComponent"];
        if (transformComponentNode) {
            auto tc = gameObject->getComponent<TransformComponent>();
            tc->translateTo(transformComponentNode["Position"].as<glm::vec3>());
            tc->setRotation(transformComponentNode["Orientation"].as<glm::vec3>());
            tc->setScale(transformComponentNode["Scale"].as<glm::vec3>());
        }

        auto ccNode = gameObjectYAML["CameraComponent"];
        if (ccNode) {
            gameObject->addComponent<CameraComponent>();
            CameraComponent *cameraComponent = gameObject->getComponent<CameraComponent>();
            cameraComponent->changeProjectionType((CameraComponent::ProjectionType) ccNode["ProjectionType"].as<int>());
            cameraComponent->setIsMainCamera(ccNode["MainCamera"].as<bool>());
            cameraComponent->setFov(ccNode["Fov"].as<float>());
            cameraComponent->setNear(ccNode["Near"].as<float>());
            cameraComponent->setFar(ccNode["Far"].as<float>());
        }

        auto meshRendererNode = gameObjectYAML["MeshRendererComponent"];
        if (meshRendererNode) {
            std::string meshName = meshRendererNode["Mesh"].as<std::string>();
            glm::vec4 color = meshRendererNode["Color"].as<glm::vec4>();
            glm::vec3 ambient = meshRendererNode["Ambient"].as<glm::vec3>();
            glm::vec3 diffuse = meshRendererNode["Diffuse"].as<glm::vec3>();
            glm::vec3 specular = meshRendererNode["Specular"].as<glm::vec3>();
            float shininess = meshRendererNode["Shininess"].as<float>();
            Material *material = new Material(color, ambient, diffuse, specular, shininess);
            Mesh *mesh;
            if (meshName == "Cube") {
                mesh = new CubeMesh();
            } else if (meshName == "Sphere") {
                mesh = new SphereMesh();
            } else if (meshName == "Cylinder") {
                mesh = new CylinderMesh();
            } else if (meshName == "Capsule") {
                mesh = new CapsuleMesh();
            } else {
                TE_LOGGER_CRITICAL("Failed to deserialize mesh renderer component.\n      Mesh name {0} is not valid.", meshName);
            }
            gameObject->addComponent<MeshRendererComponent>(mesh, material);
        }
        auto boxColliderNode = gameObjectYAML["BoxColliderComponent"];
        if (boxColliderNode) {
            gameObject->addComponent<BoxColliderComponent>();
            BoxColliderComponent *boxColliderComponent = gameObject->getComponent<BoxColliderComponent>();
            boxColliderComponent->setSize(boxColliderNode["Size"].as<glm::vec3>());
            boxColliderComponent->setOffset(boxColliderNode["Offset"].as<glm::vec3>());
            boxColliderComponent->setDynamic(boxColliderNode["IsDynamic"].as<bool>());
        }

        auto sphereColliderNode = gameObjectYAML["SphereCollider"];
        if (sphereColliderNode) {
            gameObject->addComponent<SphereCollider>();
            SphereCollider *sphereColliderComponent = gameObject->getComponent<SphereCollider>();
            sphereColliderComponent->setRadius(sphereColliderNode["Radius"].as<float>());
            sphereColliderComponent->setOffset(sphereColliderNode["Offset"].as<glm::vec3>());
            sphereColliderComponent->setDynamic(sphereColliderNode["IsDynamic"].as<bool>());
        }

        auto cylinderColliderNode = gameObjectYAML["CylinderCollider"];
        if (cylinderColliderNode) {
            gameObject->addComponent<CylinderCollider>();
            CylinderCollider *cylinderColliderComponent = gameObject->getComponent<CylinderCollider>();
            cylinderColliderComponent->setRadius(cylinderColliderNode["Radius"].as<float>());
            cylinderColliderComponent->setHeight(cylinderColliderNode["Height"].as<float>());
            cylinderColliderComponent->setOffset(cylinderColliderNode["Offset"].as<glm::vec3>());
            cylinderColliderComponent->setDynamic(cylinderColliderNode["IsDynamic"].as<bool>());
        }

        auto childrenNode = gameObjectYAML["Children"];
        for (auto childNodeYAML: childrenNode) {
            if (childrenNode) {
                deserializeGameObject(childNodeYAML, gameObject);
            }
        }
    }

    bool SceneManager::deserialize(const std::string &filepath) {
        YAML::Node data;
        try {
            data = YAML::LoadFile(filepath);
        }
        catch (YAML::Exception &e) {
            TE_LOGGER_CRITICAL("Failed to load .scene file {0}.\n      {1}", filepath, e.what());
            return false;
        }

        YAML::Node node = data["GameObjects"];
        SceneHelper::clear();
        if (node) {
            for (YAML::Node gameObjectYAML: node) {
                deserializeGameObject(gameObjectYAML, nullptr);
            }
        }

        return true;
    }

    void SceneManager::init(const std::string &projectPath) {
        for (const std::filesystem::directory_entry &p: std::filesystem::recursive_directory_iterator(projectPath)) {
            std::string path = p.path().string();
            std::string fileName = p.path().filename().string();
            if (fileName.find(".scene") != std::string::npos) {
                registerScene(path);
            }
        }
    }

    void SceneManager::registerScene(std::string &scenePath) {
        std::string sceneName = getSceneNameFromPath(scenePath);
        if (m_scenesBank.find(sceneName) != m_scenesBank.end()) {
            replaceSceneNameFromPath(scenePath);
            sceneName = getSceneNameFromPath(scenePath);
            m_scenesBank[sceneName] = scenePath;
        } else {
            m_scenesBank[sceneName] = scenePath;
        }
    }

    void SceneManager::createNewScene(std::string &scenePath) {
        std::string sceneName = getSceneNameFromPath(scenePath);
        registerScene(scenePath);
        std::filesystem::copy(Paths::scenesTemplate, scenePath);
    }

    bool SceneManager::deleteScene(std::string &scenePath) {
        std::string sceneName = getSceneNameFromPath(scenePath);
        if (m_activeSceneName == sceneName) {
            TE_LOGGER_CRITICAL("Failed to delete scene '{0}'.\n Cannot delete active scene.", sceneName);
            return false;
        } else if (m_scenesBank.find(sceneName) != m_scenesBank.end()) {
            std::filesystem::remove(scenePath);
            m_scenesBank.erase(sceneName);
            return true;
        } else {
            TE_LOGGER_CRITICAL("Failed to delete scene '{0}'", sceneName);
            return false;
        }
    }

    void SceneManager::loadScene(const std::string &sceneName) {
        if (m_scenesBank.find(sceneName) != m_scenesBank.end()) {
            if (!m_activeSceneName.empty()) {
                saveScene(m_activeSceneName);
                Scene::getInstance().clear();
            }
            std::string scenePath = m_scenesBank[sceneName];
            deserialize(scenePath);
            m_activeSceneName = sceneName;
        } else {
            TE_LOGGER_CRITICAL("Failed to load scene '{0}'.\n Could not find scene.", sceneName);
        }
    }

    void SceneManager::saveScene(const std::string &sceneName) {
        if (m_scenesBank.find(sceneName) != m_scenesBank.end()) {
            std::string scenePath = m_scenesBank[sceneName];
            serialize(sceneName, scenePath);
        } else {
            TE_LOGGER_CRITICAL("Failed to save scene '{0}'", sceneName);
        }
    }

    void SceneManager::saveCurrentScene() {
        saveScene(m_activeSceneName);
    }

    void SceneManager::saveSceneAsTemporarily(const std::string &sceneName) {
        std::string scenePath = m_scenesBank[sceneName];
        std::string sceneTemporaryPath = scenePath.substr(0, scenePath.find_last_of("\\/")) + "\\SceneTemporary.scene";
        serialize("Temporary Scene", sceneTemporaryPath);
    }

    void SceneManager::loadSceneFromTemporarily(const std::string &sceneName) {
        std::string scenePath = m_scenesBank[sceneName];
        std::string sceneTemporaryPath = scenePath.substr(0, scenePath.find_last_of("\\/")) + "\\SceneTemporary.scene";
        deserialize(sceneTemporaryPath);
        std::filesystem::remove(sceneTemporaryPath);
    }

    const std::string &SceneManager::getActiveSceneName() {
        return m_activeSceneName;
    }

    void SceneManager::replaceSceneNameFromPath(std::string &scenePath) {
        std::string sceneName = getSceneNameFromPath(scenePath);
        int i = 1;
        while (m_scenesBank.find(sceneName + std::to_string(i)) != m_scenesBank.end()) {
            i++;
        }
        scenePath = scenePath.substr(0, scenePath.find_last_of("\\/")) + "\\" + sceneName + std::to_string(i) + ".scene";
    }

    std::string SceneManager::getSceneNameFromPath(const std::string &scenePath) {
        return scenePath.substr(scenePath.find_last_of("\\/") + 1, scenePath.find_last_of('.') - scenePath.find_last_of("\\/") - 1);
    }
}