#pragma once

#include "entity/Entity.hpp"
#include "core/ExportDLL.hpp"

namespace TechEngineAPI {
    class API_DLL Component {
    protected:
        TechEngineAPI::Entity entity = 0;

    public:
        virtual ~Component() = default;

        void setEntity(Entity entity) {
            this->entity = entity;
        }
    };
}
