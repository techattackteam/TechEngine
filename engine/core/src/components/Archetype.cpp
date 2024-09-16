#include "Archetype.hpp"

namespace TechEngine {
    std::vector<ComponentTypeID> Archetype::getComponentTypes() const {
        std::vector<ComponentTypeID> types;
        for (const auto& pair: m_componentData) {
            types.push_back(pair.first);
        }
        return types;
    }

    std::vector<Entity> Archetype::getEntities() const {
        return m_entities;
    }

    void Archetype::clear() {
        m_componentData.clear();
        m_entities.clear();
    }

    void Archetype::addEntity(Entity entity) {
        if (std::find(m_entities.begin(), m_entities.end(), entity) == m_entities.end()) {
            m_entities.push_back(entity);
        }
    }

    void Archetype::removeEntity(Entity entity) {
        auto it = std::find(m_entities.begin(), m_entities.end(), entity);
        if (it != m_entities.end()) {
            size_t index = std::distance(m_entities.begin(), it);
            for (auto& pair: m_componentData) {
                ComponentTypeID typeID = pair.first;
                size_t componentSize = getComponentSize(typeID);
                auto& dataVector = pair.second;
                if (!dataVector.empty()) {
                    if (index == m_entities.size() - 1) {
                        dataVector.erase(dataVector.begin() + index * componentSize, dataVector.end());
                    } else {
                        dataVector.erase(dataVector.begin() + index * componentSize, dataVector.begin() + (index + 1) * componentSize);
                    }
                }
            }
            m_entities.erase(it);
        }
    }

    size_t Archetype::getComponentSize(ComponentTypeID typeID) {
        if (m_componentData.find(typeID) != m_componentData.end()) {
            return m_componentData[typeID].size() / m_entities.size();
        } else {
            return 0;
        }
    }

    bool Archetype::hasMatchingComponents(const std::vector<ComponentTypeID>& requiredComponents) {
        if (m_componentData.empty() && requiredComponents.empty()) {
            return true;
        } else if (m_componentData.size() == requiredComponents.size()) {
            for (ComponentTypeID typeID: requiredComponents) {
                if (m_componentData.find(typeID) == m_componentData.end()) {
                    return false;
                }
            }
            return true;
        } else {
            return false;
        }
    }

    bool Archetype::containsComponents(const std::vector<ComponentTypeID>& componentTypes) {
        for (ComponentTypeID typeID: componentTypes) {
            if (m_componentData.find(typeID) == m_componentData.end()) {
                return false;
            }
        }
        return true;
    }
}
