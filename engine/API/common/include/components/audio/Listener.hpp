#pragma once

#include <common/include/components/Component.hpp>


namespace TechEngineAPI {
    class API_DLL Listener : public Component {
    public:
        Listener(Entity entity);

        void updateInternalPointer(TechEngine::Scene* scene) override;
    };
}
