#pragma once
#include <string>
#include <utility>

#include "Component.hpp"


namespace TechEngineAPI {
    class Tag : public Component {
    private:
        std::string name;

    public:
        Tag(std::string name): name(std::move(name)) {
        }

        std::string& getName() {
            return name;
        }

        void setName(std::string name) {
            this->name = std::move(name);
        }
    };
}
