#pragma once

#include "TechEngine/core/scene/Scene.hpp"
#include "TechEngine/core/components/ArchetypesManager.hpp"

namespace TechEngine {
    class CORE_DLL Scene::Internal {
    private:
        std::string m_name = "Untitled";

        ArchetypesManager& m_archetypesManager;

        friend class SceneSerializer;
        friend class Scene;

    public:
        Internal(ArchetypesManager& archetypesManager);

        int getTotalEntities();

        std::vector<ComponentTypeID> getCommonComponents(const std::vector<Entity>& entities);

        void setName(const std::string& name);

        Internal* getImpl(Scene& scene) {
            return scene.m_internal.get();
        }

        const Internal* getImpl(const Scene& scene) {
            return scene.m_internal.get();
        }
    };
}
