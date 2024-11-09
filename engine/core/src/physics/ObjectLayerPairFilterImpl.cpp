#include "ObjectLayerPairFilterImpl.hpp"

namespace TechEngine {
    bool ObjectLayerPairFilterImpl::ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const {
        switch (inObject1) {
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING; // Non moving only collides with moving
            case Layers::MOVING:
                return inObject2 == Layers::NON_MOVING || inObject2 == Layers::MOVING || inObject2 == Layers::TRIGGER; // Moving collides with everything
            case Layers::TRIGGER:
                return inObject2 == Layers::MOVING; // Trigger only collides with moving
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
}
