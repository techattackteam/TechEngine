#include "BPLayerInterfaceImpl.hpp"

namespace TechEngine {
    BPLayerInterfaceImpl::BPLayerInterfaceImpl() {
        // Create a mapping table from object to broad phase layer
        m_ObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        m_ObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        m_ObjectToBroadPhase[Layers::TRIGGER] = BroadPhaseLayers::TRIGGER;
    }

    JPH::uint BPLayerInterfaceImpl::GetNumBroadPhaseLayers() const {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    JPH::BroadPhaseLayer BPLayerInterfaceImpl::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return m_ObjectToBroadPhase[inLayer];
    }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* BPLayerInterfaceImpl::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const {
        switch (static_cast<JPH::BroadPhaseLayer::Type>(inLayer)) {
            case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::NON_MOVING): return "NON_MOVING";
            case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::MOVING): return "MOVING";
            case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::TRIGGER): return "TRIGGER";
            default: JPH_ASSERT(false);
                return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED
}
