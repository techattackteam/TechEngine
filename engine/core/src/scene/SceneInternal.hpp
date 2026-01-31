#pragma once

#include "TechEngine/core/scene/Scene.hpp"
#include "TechEngine/core/components/ArchetypesManager.hpp"

namespace TechEngine {
    class CORE_DLL Scene::Internal {
    private:
        std::string m_name = "Untitled";

        Scene& m_scene;
        ArchetypesManager& m_archetypesManager;

        friend class SceneSerializer;
        friend class Scene;

    public:
        Internal(Scene& scene, ArchetypesManager& archetypesManager);

        int getTotalEntities();

        std::vector<ComponentTypeID> getCommonComponents(const std::vector<Entity>& entities);

        template<typename T>
        uint32_t getComponentCount() {
            return m_archetypesManager.getComponentCount<T>();
        }

        void setName(const std::string& name);

        void updateGlobalTransforms();

        Internal* getImpl(Scene& scene) {
            return scene.m_internal.get();
        }

        const Internal* getImpl(const Scene& scene) {
            return scene.m_internal.get();
        }

    private:
        void updateTransformsRecursive(Entity entity, const glm::mat4& parentTransformNoScale, const glm::vec3& parentWorldScale, bool parentDirty);
    };
}
