#pragma once

#include "components/Components.hpp"

#include <Jolt/Physics/Collision/Shape/MutableCompoundShape.h>

namespace TechEngine {
    enum class Shape {
        Cube,
        Sphere,
        Capsule,
        Cylinder
    };

    namespace ShapeFactory {
        JPH::MutableCompoundShape* createShape(Shape shape, const Transform& transform, glm::vec3 center, glm::vec3 size);

        JPH::MutableCompoundShape* createBoxShape(const Transform& transform, glm::vec3 center, glm::vec3 scale);

        JPH::MutableCompoundShape* createSphereShape(const Transform& transform, glm::vec3 center, float radius);

        JPH::MutableCompoundShape* createCapsuleShape(const Transform& transform, glm::vec3 center, const float height, const float radius);

        JPH::MutableCompoundShape* createCylinderShape(const Transform& transform, glm::vec3 center, float height, float radius);
    }
}
