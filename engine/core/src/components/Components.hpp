#pragma once
#include <glm/vec3.hpp>
#include <utility>
#include <string>

namespace TechEngine {
    class Tag {
    public:
        const std::string uuid;
        std::string name;

        explicit Tag(std::string name, std::string tag) : name(std::move(name)), uuid(std::move(tag)) {
        }
    };

    class Transform {
    public:
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f); // Maybe use quaternions instead
        glm::vec3 scale = glm::vec3(1.0f);
    };

}
