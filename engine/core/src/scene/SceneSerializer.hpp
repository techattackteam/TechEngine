#pragma once

#include <filesystem>
#include <string>
#include <yaml-cpp/emitter.h>

#include "components/Components.hpp"

namespace TechEngine {
    class ResourcesManager;
    class Scene;
    class Archetype;

    class SceneSerializer {
    private:
        Scene& m_scene;
        ResourcesManager& m_resourcesManager;

    public:
        SceneSerializer(Scene& scene, ResourcesManager& resourcesManager);

        void serialize(std::ofstream& stream) const;

        std::string getSceneName(const std::filesystem::path& path);

        void deserialize(const std::filesystem::path& path) const;

    private:
        void serializeComponent(Archetype& archetype, const Entity& entity, const ComponentTypeID& typeID, YAML::Emitter& out) const;

        void deserializeComponentNode(const Entity& entity, const YAML::Node& componentNode) const;
    };
}
