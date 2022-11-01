#include "SceneSerializer.hpp"

#include "scene/GameObject.hpp"
#include "components/CameraComponent.hpp"
#include "components/TransformComponent.hpp"
#include "Scene.hpp"
#include "components/MeshRendererComponent.hpp"
#include "mesh/CubeMesh.hpp"

#include <fstream>
#include <filesystem>

#include <yaml-cpp/yaml.h>
#include <iostream>

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


    static void SerializeGameObject(YAML::Emitter &out, GameObject *gameObject) {
        out << YAML::BeginMap;
        out << YAML::Key << "GameObject" << YAML::Value << gameObject->getName();

        out << YAML::Key << "Name";
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << gameObject->getName();
        out << YAML::EndMap;

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
            out << YAML::EndMap;
        }

        if (gameObject->hasComponent<MeshRendererComponent>()) {
            out << YAML::Key << "MeshRendererComponent";
            out << YAML::BeginMap;
            auto meshRendererComponent = gameObject->getComponent<MeshRendererComponent>();
            Material &material = meshRendererComponent->getMaterial();
            out << YAML::Key << "Color" << YAML::Value << material.getColor();
            out << YAML::Key << "Ambient" << YAML::Value << material.getAmbient();
            out << YAML::Key << "Diffuse" << YAML::Value << material.getDiffuse();
            out << YAML::Key << "Specular" << YAML::Value << material.getSpecular();
            out << YAML::Key << "Shininess" << YAML::Value << material.getShininess();
            out << YAML::EndMap;
        }
        out << YAML::EndMap;
    }

    void SceneSerializer::serialize(const std::string &filepath) {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Scene" << YAML::Value << Scene::getInstance().getName();
        out << YAML::Key << "GameObjects" << YAML::Value << YAML::BeginSeq;
        for (GameObject *gameObject: Scene::getInstance().getGameObjects()) {
            SerializeGameObject(out, gameObject);
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::filesystem::create_directories("project/scenes");
        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    bool SceneSerializer::deserialize(const std::string &filepath) {
        YAML::Node data;
        try {
            data = YAML::LoadFile(filepath);
        }
        catch (YAML::Exception &e) {
            std::cout << "Failed to load .scene file " << filepath << "\n     " << e.what() << std::endl;
            return false;
        }

        if (!data["Scene"])
            return false;

        std::string sceneName = data["Scene"].as<std::string>();

        auto node = data["GameObjects"];
        //Should clear scene
        Scene::getInstance().clear();
        if (node) {
            for (auto gameObjectYAML: node) {

                std::string name;
                auto tagComponent = gameObjectYAML["Name"];
                name = tagComponent["name"].as<std::string>();
                GameObject *gameObject = new GameObject(name);

                auto transformComponentNode = gameObjectYAML["TransformComponent"];
                if (transformComponentNode) {
                    auto tc = gameObject->getComponent<TransformComponent>();
                    tc->translateTo(transformComponentNode["Position"].as<glm::vec3>());
                    tc->rotate(transformComponentNode["Orientation"].as<glm::vec3>());
                    tc->setScale(transformComponentNode["Scale"].as<glm::vec3>());
                }

                auto ccNode = gameObjectYAML["CameraComponent"];
                if (ccNode) {
                    gameObject->addComponent<CameraComponent>(gameObject);
                    CameraComponent *cameraComponent = gameObject->getComponent<CameraComponent>();
                    cameraComponent->changeProjectionType((CameraComponent::ProjectionType) ccNode["ProjectionType"].as<int>());
                    cameraComponent->setIsMainCamera(ccNode["MainCamera"].as<bool>());
                }

                auto meshRendererNode = gameObjectYAML["MeshRendererComponent"];
                if (meshRendererNode) {
                    glm::vec4 color = meshRendererNode["Color"].as<glm::vec4>();
                    glm::vec3 ambient = meshRendererNode["Ambient"].as<glm::vec3>();
                    glm::vec3 diffuse = meshRendererNode["Diffuse"].as<glm::vec3>();
                    glm::vec3 specular = meshRendererNode["Specular"].as<glm::vec3>();
                    float shininess = meshRendererNode["Shininess"].as<float>();
                    Material *material = new Material(color, ambient, diffuse, specular, shininess);
                    gameObject->addComponent<MeshRendererComponent>(gameObject, new CubeMesh(), material);
                }
            }
        }

        return true;
    }
}