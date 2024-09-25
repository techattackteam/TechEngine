#pragma once

#include "components/Component.hpp"
#include "entity/Entity.hpp"
#include "core/ExportDLL.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace TechEngine {
    class Tag;
    class Scene;
}

namespace TechEngineAPI {
    class API_DLL Scene {
    private:
        friend class Entry;
        friend class ClientEntry;
        friend class ServerEntry;

        inline static std::unordered_map<TechEngineAPI::Entity, std::shared_ptr<Component>> components;

        inline static TechEngine::Scene* scene;

        static void init(TechEngine::Scene* scene);

        static void shutdown();

        static void updateComponents();

        static void sendUpdatedComponents();

    public:
        static Entity createEntity(const std::string& name);

        static std::vector<Entity> getEntitiesWithName(const std::string& name);

        static void destroyEntity(Entity entity);

        template<typename T>
        static std::shared_ptr<T> getComponent(Entity entity);

        template<typename T>
        static T& getComponentInternal(Entity entity);
    };
}
