#pragma once

#include "common/include/core/ExportDLL.hpp"
#include "common/include/components/Component.hpp"
#include "common/include/entity/Entity.hpp"

#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace TechEngine {
    class Tag;
    class Scene;
    class ResourcesManager;
}

namespace TechEngineAPI {
    struct ComponentKey {
        Entity entity;
        std::type_index type;

        bool operator==(const ComponentKey& rhs) const {
            return entity == rhs.entity && type == rhs.type;
        }
    };

    struct ComponentKeyHash {
        std::size_t operator()(const ComponentKey& key) const {
            std::size_t h1 = std::hash<Entity>()(key.entity);
            std::size_t h2 = key.type.hash_code();
            return h1 ^ (h2 << 1); // Shift and XOR combination
        }
    };

    class API_DLL Scene {
    private:
        friend class Entry;
        friend class ClientEntry;
        friend class ServerEntry;

        inline static std::unordered_map<ComponentKey, std::shared_ptr<Component>, ComponentKeyHash> components;

        inline static TechEngine::Scene* m_scene;
        inline static TechEngine::ResourcesManager* m_resourcesManager;

        static void init(TechEngine::Scene* scene, TechEngine::ResourcesManager* m_resourcesManager);

        static void shutdown();

    public:
        static Entity createEntity(const std::string& name);

        static std::vector<Entity> getEntitiesWithName(const std::string& name);

        static void destroyEntity(Entity entity);

        template<typename T, typename... Args>
        static std::shared_ptr<T> addComponent(Entity entity, Args&&... args);

        template<typename T, typename... Args>
        static std::shared_ptr<T> addComponentInternal(Entity entity, Args&&... args);

        template<typename T>
        static void removeComponent(Entity entity);

        template<typename T>
        static std::shared_ptr<T> getComponent(Entity entity);

        template<typename T>
        static T& getComponentInternal(Entity entity);
    };
}
