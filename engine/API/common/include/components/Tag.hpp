#pragma once
#include <string>
#include <utility>

#include "Component.hpp"


namespace TechEngine {
    class Tag;
}

namespace TechEngineAPI {
    class API_DLL Tag : public Component {
    private:
        TechEngine::Tag* m_tag;
        std::string name;

    public:
        Tag(Entity entity, std::string name): Component(entity), name(std::move(name)) {
        }

        void updateInternalPointer(TechEngine::Scene* scene) override;

        std::string& getName() {
            return name;
        }

        void setName(std::string name) {
            this->name = std::move(name);
        }
    };
}
