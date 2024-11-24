#pragma once

#include <common/include/components/Component.hpp>

namespace TechEngineAPI {
    class RigidBody : public Component {
    public:
        RigidBody(Entity entity);

        void updateInternalPointer(TechEngine::Scene* scene) override {
        }
    };
}
