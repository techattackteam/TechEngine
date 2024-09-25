#pragma once

#include "entity/Entity.hpp"

namespace TechEngineAPI {
    class Component {
    protected:
        TechEngineAPI::Entity entity = 0;

    public:
        virtual ~Component() = default;

        void setEntity(Entity entity) {
            this->entity = entity;
        }
    };
}
