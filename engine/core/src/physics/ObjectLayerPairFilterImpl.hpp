#pragma once

#include "Layers.hpp"
#include <Jolt/Physics/Collision/ObjectLayer.h>

namespace TechEngine {
    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override;
    };
}
