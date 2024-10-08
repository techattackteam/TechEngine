#pragma once

#include "common/include/entity/Entity.hpp"
#include "common/include/core/ExportDLL.hpp"

namespace TechEngine {
    class Scene;
}

namespace TechEngineAPI {
    class API_DLL Component {
    protected:
        TechEngineAPI::Entity m_entity = 0;

    public:
        Component(Entity entity): m_entity(entity) {
        };

        virtual ~Component() = default;

        void setEntity(Entity entity) {
            this->m_entity = entity;
        }

        virtual void updateInternalPointer(TechEngine::Scene* scene) = 0;
    };
}
