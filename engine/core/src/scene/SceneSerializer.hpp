#pragma once

#include "components/Archetype.hpp"
#include <filesystem>
#include <yaml-cpp/emitter.h>

namespace TechEngine {
    class ResourcesManager;
    class Scene;
    class Archetype;
    class PhysicsEngine;

    class SceneSerializer {
    private:
        Scene& m_scene;
        ResourcesManager& m_resourcesManager;
        PhysicsEngine& m_physicsEngine;

    public:
        SceneSerializer(Scene& scene, ResourcesManager& resourcesManager, PhysicsEngine& physicsEngine);

        void serialize(std::ofstream& stream) const;

        std::string getSceneName(const std::filesystem::path& path);

        void deserialize(const std::filesystem::path& path) const;

    private:
        void serializeComponent(const Entity& entity, const ComponentTypeID& typeID, YAML::Emitter& out) const;

        void deserializeComponentNode(const Entity& entity, const YAML::Node& componentNode) const;
    };
}
