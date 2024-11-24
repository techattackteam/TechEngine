#pragma once
#include <common/include/components/Component.hpp>

namespace TechEngineAPI {
    class KinematicBody : public Component {
    public:
        KinematicBody(Entity entity);

        void updateInternalPointer(TechEngine::Scene* scene) override {
        }
    };
}
