#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/scene/Scene.hpp>

#include <scene/ArchetypeStorage.hpp>
#include <scene/components/Hierarchy.hpp>

#include <cstddef>
#include <memory>
#include <vector>

namespace TechEngine {
    Scene::Scene(ComponentRegistry& registry) : m_storage(std::make_unique<ArchetypeStorage>(registry)) {
    }

    Scene::~Scene() = default;

    Entity Scene::createEntity() const {
        TE_PROFILER_FUNCTION();
        return m_storage->createEntity();
    }
    bool Scene::destroyEntity(Entity entity) {
        TE_PROFILER_FUNCTION();
        Hierarchy* rootHierarchy = getHierarchy(entity);
        if (rootHierarchy == nullptr) {
            return false;
        }

        std::vector<Entity> subtree;
        subtree.push_back(entity);
        for (std::size_t index = 0; index < subtree.size(); index++) {
            const Hierarchy* hierarchy = getHierarchy(subtree[index]);
            if (hierarchy == nullptr) {
                return false;
            }

            Entity child = hierarchy->m_firstChild;
            while (child.valid()) {
                const Hierarchy* childHierarchy = getHierarchy(child);
                if (childHierarchy == nullptr) {
                    return false;
                }
                subtree.push_back(child);
                child = childHierarchy->m_nextSibling;
            }
        }

        if (rootHierarchy->m_parent.valid() && !unparent(entity)) {
            return false;
        }

        for (auto it = subtree.rbegin(); it != subtree.rend(); it++) {
            const bool removed = m_storage->destroyEntity(*it);
            TE_CHECK(removed, "Collected subtree entity became invalid during destruction");
        }
        return true;
    }

    bool Scene::contains(const Entity entity) const {
        return m_storage->contains(entity);
    }

    void Scene::clear() {
        TE_PROFILER_FUNCTION();
        m_storage->clear();
    }
    Entity Scene::getParent(Entity entity) const {
        const Hierarchy* hierarchy = this->getHierarchy(entity);

        return hierarchy != nullptr ? hierarchy->m_parent : Entity{};
    }
    std::vector<Entity> Scene::getRoots() {
        TE_PROFILER_FUNCTION();
        std::vector<Entity> result;
        m_storage->eachEntity([&](Entity entity) {
            if (!getParent(entity).valid()) {
                result.push_back(entity);
            }
        });
        return result;
    }

    std::vector<Entity> Scene::getChildren(Entity parent) const {
        TE_PROFILER_FUNCTION();
        std::vector<Entity> result;
        const Hierarchy* parentHierarchy = this->getHierarchy(parent);
        if (parentHierarchy == nullptr) {
            return result;
        }

        Entity currentChild = parentHierarchy->m_firstChild;
        while (currentChild.valid()) {
            result.push_back(currentChild);
            const Hierarchy* currentChildHierarchy = this->getHierarchy(currentChild);
            if (currentChildHierarchy != nullptr) {
                currentChild = currentChildHierarchy->m_nextSibling;
            } else {
                break;
            }
        }
        return result;
    }

    bool Scene::setParent(const Entity child, const Entity parent, const std::size_t position) {
        TE_PROFILER_FUNCTION();
        if (child == parent) {
            return false;
        }

        Hierarchy* childHierarchy = getHierarchy(child);
        Hierarchy* parentHierarchy = getHierarchy(parent);
        if (childHierarchy == nullptr || parentHierarchy == nullptr) {
            return false;
        }

        for (Entity ancestor = parent; ancestor.valid();) {
            if (ancestor == child) {
                return false;
            }
            const Hierarchy* ancestorHierarchy = getHierarchy(ancestor);
            if (ancestorHierarchy == nullptr) {
                return false;
            }
            ancestor = ancestorHierarchy->m_parent;
        }

        const Entity oldParent = childHierarchy->m_parent;
        const Entity oldPrevious = childHierarchy->m_previousSibling;
        const Entity oldNext = childHierarchy->m_nextSibling;
        Hierarchy* oldParentHierarchy = oldParent.valid() ? getHierarchy(oldParent) : nullptr;
        Hierarchy* oldPreviousHierarchy = oldPrevious.valid() ? getHierarchy(oldPrevious) : nullptr;
        Hierarchy* oldNextHierarchy = oldNext.valid() ? getHierarchy(oldNext) : nullptr;

        if (oldParent.valid()) {
            if (oldParentHierarchy == nullptr || oldParentHierarchy->m_childrenCount == 0 || (oldPrevious.valid() && oldPreviousHierarchy == nullptr) || (oldNext.valid() && oldNextHierarchy == nullptr)) {
                return false;
            }
            if ((oldPrevious.valid() && oldPreviousHierarchy->m_nextSibling != child) || (!oldPrevious.valid() && oldParentHierarchy->m_firstChild != child) || (oldNext.valid() && oldNextHierarchy->m_previousSibling != child)) {
                return false;
            }
        } else if (oldPrevious.valid() || oldNext.valid()) {
            return false;
        }

        const std::size_t destinationCount = parentHierarchy->m_childrenCount - static_cast<std::size_t>(oldParent == parent);
        if (position > destinationCount) {
            return false;
        }

        Entity insertionBefore;
        Entity insertionAfter;
        std::size_t visited = 0;
        for (Entity current = parentHierarchy->m_firstChild; current.valid();) {
            const Hierarchy* currentHierarchy = getHierarchy(current);
            if (currentHierarchy == nullptr) {
                return false;
            }
            if (current != child) {
                if (visited == position) {
                    insertionAfter = current;
                    break;
                }
                insertionBefore = current;
                visited++;
            }
            current = currentHierarchy->m_nextSibling;
        }
        if (!insertionAfter.valid() && visited != position) {
            return false;
        }

        Hierarchy* beforeHierarchy = insertionBefore.valid() ? getHierarchy(insertionBefore) : nullptr;
        Hierarchy* afterHierarchy = insertionAfter.valid() ? getHierarchy(insertionAfter) : nullptr;
        if ((insertionBefore.valid() && beforeHierarchy == nullptr) || (insertionAfter.valid() && afterHierarchy == nullptr)) {
            return false;
        }

        if (oldParent.valid()) {
            if (oldPreviousHierarchy != nullptr) {
                oldPreviousHierarchy->m_nextSibling = oldNext;
            } else {
                oldParentHierarchy->m_firstChild = oldNext;
            }
            if (oldNextHierarchy != nullptr) {
                oldNextHierarchy->m_previousSibling = oldPrevious;
            }
            oldParentHierarchy->m_childrenCount--;
        }

        childHierarchy->m_parent = parent;
        childHierarchy->m_previousSibling = insertionBefore;
        childHierarchy->m_nextSibling = insertionAfter;
        if (beforeHierarchy != nullptr) {
            beforeHierarchy->m_nextSibling = child;
        } else {
            parentHierarchy->m_firstChild = child;
        }
        if (afterHierarchy != nullptr) {
            afterHierarchy->m_previousSibling = child;
        }
        parentHierarchy->m_childrenCount++;
        return true;
    }
    bool Scene::unparent(Entity child) {
        Hierarchy* childHierarchy = this->getHierarchy(child);
        if (childHierarchy == nullptr || !childHierarchy->m_parent.valid()) {
            return false;
        }

        Hierarchy* parentHierarchy = this->getHierarchy(childHierarchy->m_parent);
        if (parentHierarchy != nullptr) {
            if (parentHierarchy->m_firstChild == child) {
                parentHierarchy->m_firstChild = childHierarchy->m_nextSibling;
            }
            if (childHierarchy->m_previousSibling.valid()) {
                Hierarchy* previousSiblingHierarchy = this->getHierarchy(childHierarchy->m_previousSibling);
                if (previousSiblingHierarchy != nullptr) {
                    previousSiblingHierarchy->m_nextSibling = childHierarchy->m_nextSibling;
                }
            }
            if (childHierarchy->m_nextSibling.valid()) {
                Hierarchy* nextSiblingHierarchy = this->getHierarchy(childHierarchy->m_nextSibling);
                if (nextSiblingHierarchy != nullptr) {
                    nextSiblingHierarchy->m_previousSibling = childHierarchy->m_previousSibling;
                }
            }
            parentHierarchy->m_childrenCount--;
        }

        childHierarchy->m_parent = Entity();
        childHierarchy->m_previousSibling = Entity();
        childHierarchy->m_nextSibling = Entity();

        return true;
    }
    bool Scene::reorderChild(Entity child, std::size_t position) {
        Hierarchy* childHierarchy = this->getHierarchy(child);
        if (childHierarchy == nullptr || !childHierarchy->m_parent.valid()) {
            return false;
        }

        return setParent(child, childHierarchy->m_parent, position);
    }

    Hierarchy* Scene::getHierarchy(const Entity entity) {
        return m_storage->component<Hierarchy>(entity);
    }

    const Hierarchy* Scene::getHierarchy(const Entity entity) const {
        return m_storage->component<Hierarchy>(entity);
    }
}
