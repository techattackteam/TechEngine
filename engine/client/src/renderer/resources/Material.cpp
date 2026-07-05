#include "Material.hpp"

namespace TechEngine {
    Material::Material(const MaterialProperties& properties, const UUID& resourceUUID) : m_properties(properties), IGPUResource(resourceUUID) {
    }

    void Material::setSSBOSlot(uint32_t slot) {
        this->m_ssboSlot = slot;
    }

    const uint32_t Material::getSSBOSlot() const {
        return this->m_ssboSlot;
    }

    const MaterialProperties& Material::getProperties() {
        return m_properties;
    }
}
