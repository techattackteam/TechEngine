#pragma once

#include "components/Component.hpp"

namespace TechEngine {
    class NetworkSync : public Component {
    public:
        explicit NetworkSync(GameObject* gameObject);

        ~NetworkSync() override;

        void update() override;

        void fixedUpdate() override;

        Component* copy(GameObject* gameObjectToAttach, Component* componentToCopy) override;
    };
}
