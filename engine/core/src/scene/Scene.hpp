#pragma once
#include <complex.h>
#include <vector>

#include "components/ComponentsManager.hpp"
#include "systems/System.hpp"

namespace TechEngine {
    using Entity = int;

    class Scene : public System {
    private:
        Entity nextEntityId = 0;
        std::vector<Entity> freeEntities;
        std::vector<Entity> entities;
        ComponentsManager componentsManager;

    public:
        void init() override;

        int createEntity(const std::string& name);

        void destroyEntity(Entity entity);

        const std::vector<Entity>& getEntities() const;

        std::vector<std::any> getEntityComponents(Entity entity);

        template<typename T, typename... Args>
        T& addComponent(Entity entity, Args&&... args) {
            return componentsManager.addComponent<T>(entity, std::forward<Args>(args)...);
        }

        template<typename T>
        void removeComponent(Entity entity) {
            componentsManager.removeComponent<T>(entity);
        }

        template<typename T>
        std::vector<T> getComponents() {
            return componentsManager.getAllComponents<T>();
        }

        template<typename T>
        T& getComponent(Entity entity) {
            return componentsManager.getComponent<T>(entity);
        }

        std::vector<std::string> getCommonComponents(const std::vector<Entity>& entities);

        template<typename T>
        bool hasComponent(Entity entity) {
            return componentsManager.hasComponent<T>(entity);
        }

        int getEntityFromComponent(Camera* camera);

        bool hasMainCamera();

        Camera& getMainCamera();
    };
}
