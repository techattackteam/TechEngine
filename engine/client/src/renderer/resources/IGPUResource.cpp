#include "IGPUResource.hpp"

namespace TechEngine {
    IGPUResource::IGPUResource(const UUID& resourceUUID) : m_resourceUUID(resourceUUID) {
    }

    const UUID& IGPUResource::getUUID() const {
        return m_resourceUUID;
    }
}
