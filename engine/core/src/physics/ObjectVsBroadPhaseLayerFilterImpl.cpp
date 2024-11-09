#include "ObjectVsBroadPhaseLayerFilterImpl.hpp"

namespace TechEngine {
    bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const {
        switch (inLayer1) {
            case Layers::NON_MOVING:
                return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return inLayer2 == BroadPhaseLayers::NON_MOVING || inLayer2 == BroadPhaseLayers::MOVING || inLayer2 == BroadPhaseLayers::TRIGGER; // Moving collides with everything
            case Layers::TRIGGER:
                return inLayer2 == BroadPhaseLayers::MOVING;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
}
