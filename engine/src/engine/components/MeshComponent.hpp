#pragma once

#include "Component.hpp"

namespace Engine {
    class MeshComponent : public Component {
    public:
        MeshComponent();

        void getInfo() override;
    };
}


