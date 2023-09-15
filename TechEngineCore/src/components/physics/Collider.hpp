#pragma once

#include "components/Component.hpp"

namespace TechEngine {

    class Collider : public Component {
    private:
        bool dynamic = true;
    public:
        Collider(GameObject *gameObject, std::string name);

        Collider(GameObject *gameObject, std::string name, bool dynamic);

        void setDynamic(bool dynamic);

        bool isDynamic();
    };
}
