#pragma once

#include <algorithm>
#include <any>
#include <cassert>
#include <typeindex>
#include <unordered_map>

#include "Components.hpp"
#include "core/Logger.hpp"

namespace TechEngine {
    using Entity = int;

    class ComponentsManager {
    private:
        // Map from type index (component type) to vector of components
        std::unordered_map<std::type_index, std::vector<std::any>> componentPools;
        std::unordered_map<std::type_index, std::unordered_map<Entity, size_t>> entityToIndexMap;
        const std::vector<std::string> registeredComponents = {
            std::type_index(typeid(Tag)).name(),
            std::type_index(typeid(Transform)).name(),
        };

    public:
        ~ComponentsManager() {
            clear();
        }

        void init() {
            clear();
        }

        template<typename T>
        bool isComponentRegistered() {
            return std::find(registeredComponents.begin(), registeredComponents.end(), typeid(T).name()) != registeredComponents.end();
        }

        template<typename T, typename... Args>
        T& addComponent(Entity entity, Args&&... args) {
            assert(isComponentRegistered<T>() && "Component type not registered");
            auto& pool = componentPools[typeid(T)];
            auto& entityMap = entityToIndexMap[typeid(T)];

            pool.emplace_back(T(args...));
            entityMap[entity] = pool.size() - 1;
            return std::any_cast<T&>(pool.back());
        }

        template<typename T>
        void removeComponent(Entity entity) {
            assert(isComponentRegistered<T>() && "Component type not registered");
            auto& pool = componentPools[typeid(T)];
            auto& entityMap = entityToIndexMap[typeid(T)];

            assert(entityMap.find(entity) != entityMap.end() && "Entity does not have this component");
            size_t index = entityMap[entity];
            pool[index] = pool.back();
            entityMap[entity] = pool.size() - 1;
            pool.pop_back();
        }

        template<typename T>
        bool hasComponent(Entity entity) {
            assert(isComponentRegistered<T>() && "Component type not registered");
            return entityToIndexMap.find(typeid(T)) != entityToIndexMap.end() && entityToIndexMap[typeid(T)].find(entity) != entityToIndexMap[typeid(T)].end();
        };

        template<typename T>
        T& getComponent(Entity entity) {
            assert(isComponentRegistered<T>() && "Component type not registered");
            auto& pool = componentPools[typeid(T)];
            auto& entityMap = entityToIndexMap[typeid(T)];

            assert(entityMap.find(entity) != entityMap.end() && "Entity does not have this component");
            return std::any_cast<T&>(pool[entityMap[entity]]);
        }

        template<typename T>
        std::vector<T> getAllComponents() {
            assert(isComponentRegistered<T>() && "Component type not registered");
            std::vector<T> result;
            for (const auto& component: componentPools[typeid(T)]) {
                result.push_back(std::any_cast<T>(component));
            }
            return result;
        }

        std::vector<std::any> getEntityComponents(Entity entity) {
            std::vector<std::any> result;
            for (auto& pair: componentPools) {
                auto& pool = pair.second;
                auto& entityMap = entityToIndexMap[pair.first];

                if (entityMap.find(entity) != entityMap.end()) {
                    result.push_back(pool[entityMap[entity]]);
                }
            }
            return result;
        }

        void removeEntityComponents(Entity entity) {
            for (auto& pair: componentPools) {
                auto& pool = pair.second;
                auto& entityMap = entityToIndexMap[pair.first];

                if (entityMap.find(entity) != entityMap.end()) {
                    size_t index = entityMap[entity];
                    pool[index] = pool.back();
                    entityMap[entity] = pool.size() - 1;
                    pool.pop_back();
                }
            }
        }

        void clear() {
            componentPools.clear();
            entityToIndexMap.clear();
        }

        std::vector<std::string> getCommonComponents(const std::vector<Entity>& entities) {
            std::vector<std::string> result;
            for (auto& pair: componentPools) {
                auto& pool = pair.second;
                auto& entityMap = entityToIndexMap[pair.first];

                for (Entity entity: entities) {
                    if (entityMap.find(entity) == entityMap.end()) {
                        goto nextComponent;
                    }
                }
                result.emplace_back(pair.first.name());
            nextComponent:;
            }
            return result;
        }
    };
}
