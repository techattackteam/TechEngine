#include "ShapeFactory.hpp"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

namespace TechEngine::ShapeFactory {
    JPH::MutableCompoundShape* createShape(const Shape shape, const Transform& transform, const glm::vec3 center, const glm::vec3 size) {
        JPH::MutableCompoundShape* compoundShape = nullptr;
        if (shape == Shape::Cube) {
            compoundShape = createBoxShape(transform, center, size);
        } else if (shape == Shape::Sphere) {
            compoundShape = createSphereShape(transform, center, size.x);
        } else if (shape == Shape::Capsule) {
            compoundShape = createCapsuleShape(transform, center, size.y, size.x);
        } else if (shape == Shape::Cylinder) {
            compoundShape = createCylinderShape(transform, center, size.y, size.x);
            TE_LOGGER_INFO("Cylinder shape created with height {0} and radius {1}", size.y, size.x);
        } else {
            TE_LOGGER_CRITICAL("Shape not supported");
            return nullptr;
        }
        return compoundShape;
    }

    JPH::MutableCompoundShape* createBoxShape(const Transform& transform, glm::vec3 center, glm::vec3 scale) {
        JPH::RegisterDefaultAllocator();
        JPH::MutableCompoundShape* compoundShape = new JPH::MutableCompoundShape();
        JPH::Ref<JPH::BoxShape> shape = new JPH::BoxShape(JPH::Vec3(0.5f, 0.5f, 0.5f));
        JPH::Ref<JPH::ScaledShape> scaledShape = new JPH::ScaledShape(shape, JPH::RVec3(transform.scale.x * scale.x, transform.scale.y * scale.y, transform.scale.z * scale.z));
        compoundShape->AddShape(JPH::RVec3(center.x, center.y, center.z), JPH::Quat::sIdentity(), scaledShape);
        return compoundShape;
    }

    JPH::MutableCompoundShape* createSphereShape(const Transform& transform, const glm::vec3 center, const float radius) {
        JPH::RegisterDefaultAllocator();
        JPH::MutableCompoundShape* compoundShape = new JPH::MutableCompoundShape();
        float transformScale = std::max(transform.scale.x, std::max(transform.scale.y, transform.scale.z));
        const JPH::Ref<JPH::SphereShape> shape = new JPH::SphereShape(radius);
        const JPH::Ref<JPH::ScaledShape> scaledShape = new JPH::ScaledShape(shape, JPH::RVec3(transformScale, transformScale, transformScale));
        compoundShape->AddShape(JPH::RVec3(center.x, center.y, center.z), JPH::Quat::sIdentity(), scaledShape);
        return compoundShape;
    }

    JPH::MutableCompoundShape* createCapsuleShape(const Transform& transform, const glm::vec3 center, const float height, const float radius) {
        JPH::RegisterDefaultAllocator();
        JPH::MutableCompoundShape* compoundShape = new JPH::MutableCompoundShape();
        float transformScale = std::max(transform.scale.x, std::max(transform.scale.y, transform.scale.z));
        JPH::Ref<JPH::CapsuleShape> shape = new JPH::CapsuleShape(height * 0.5f, radius);
        JPH::Ref<JPH::ScaledShape> scaledShape = new JPH::ScaledShape(shape, JPH::RVec3(transformScale, transformScale, transformScale));
        compoundShape->AddShape(JPH::RVec3(center.x, center.y, center.z), JPH::Quat::sIdentity(), scaledShape);
        return compoundShape;
    }

    JPH::MutableCompoundShape* createCylinderShape(const Transform& transform, const glm::vec3 center, const float height, const float radius) {
        JPH::RegisterDefaultAllocator();
        JPH::MutableCompoundShape* compoundShape = new JPH::MutableCompoundShape();
        JPH::Ref<JPH::CylinderShape> shape = new JPH::CylinderShape(height * 0.5f, radius);
        JPH::Ref<JPH::ScaledShape> scaledShape = new JPH::ScaledShape(shape, JPH::RVec3(transform.scale.x, transform.scale.y, transform.scale.z));
        compoundShape->AddShape(JPH::RVec3(center.x, center.y, center.z), JPH::Quat::sIdentity(), scaledShape);
        return compoundShape;
    }
}
