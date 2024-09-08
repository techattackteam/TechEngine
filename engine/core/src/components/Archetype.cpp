#include "Archetype.hpp"

namespace TechEngine {
    std::vector<ComponentTypeID> Archetype::getComponentTypes() const {
        std::vector<ComponentTypeID> types;
        for (const auto& pair: componentData) {
            types.push_back(pair.first);
        }
        return types;
    }

    std::vector<Entity> Archetype::getEntities() const {
        return entities;
    }

    void Archetype::addEntity(Entity entity) {
        if (std::find(entities.begin(), entities.end(), entity) == entities.end()) {
            entities.push_back(entity);
        }
    }

    void Archetype::removeEntity(Entity entity) {
        auto it = std::find(entities.begin(), entities.end(), entity);
        if (it != entities.end()) {
            size_t index = std::distance(entities.begin(), it);
            for (auto& pair: componentData) {
                ComponentTypeID typeID = pair.first;
                size_t componentSize = getComponentSize(typeID);
                auto& dataVector = pair.second;
                if (!dataVector.empty()) {
                    if (index == entities.size() - 1) {
                        dataVector.erase(dataVector.begin() + index * componentSize, dataVector.end());
                    } else {
                        dataVector.erase(dataVector.begin() + index * componentSize, dataVector.begin() + (index + 1) * componentSize);
                    }
                }
            }
            entities.erase(it);
        }
    }

    size_t Archetype::getComponentSize(ComponentTypeID typeID) {
        if (componentData.find(typeID) != componentData.end()) {
            return componentData[typeID].size() / entities.size();
        } else {
            return 0;
        }
    }

    bool Archetype::hasMatchingComponents(const std::vector<ComponentTypeID>& requiredComponents) {
        if (componentData.empty() && requiredComponents.empty()) {
            return true;
        } else if (componentData.size() == requiredComponents.size()) {
            for (ComponentTypeID typeID: requiredComponents) {
                if (componentData.find(typeID) == componentData.end()) {
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
            if (componentData.find(typeID) == componentData.end()) {
                return false;
            }
        }
        return true;
    }
}
