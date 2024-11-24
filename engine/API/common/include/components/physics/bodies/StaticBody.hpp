#pragma once

#include <common/include/components/Component.hpp>

namespace TechEngineAPI {
    class StaticBody : public Component {
    public:
        explicit StaticBody(Entity entity);

        void updateInternalPointer(TechEngine::Scene* scene) override {
        };
    };
}
