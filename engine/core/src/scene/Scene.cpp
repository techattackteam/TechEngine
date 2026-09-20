#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneCommandBuffer.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/systems/ScheduleAccess.hpp>

#include <scene/ArchetypeStorage.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

namespace TechEngine {
    struct SceneExecutionState {
        Scene* scene = nullptr;
        const ScheduleAccess* access = nullptr;
        SceneCommandBuffer* commands = nullptr;
    };

    static thread_local SceneExecutionState g_sceneExecutionState;

    static Mat4 transformMatrix(const TransformValues& values) {
        return glm::translate(Mat4(1.0f), values.position) * glm::mat4_cast(values.rotation) * glm::scale(Mat4(1.0f), values.scale);
    }

    static bool decomposeLocalMatrix(const Mat4& matrix, TransformValues& result) {
        constexpr float tolerance = 0.0001f;
        Vec3 axes[3] = {Vec3(matrix[0]), Vec3(matrix[1]), Vec3(matrix[2])};
        Vec3 scale;
        for (int axis = 0; axis < 3; axis++) {
            scale[axis] = glm::length(axes[axis]);
            if (!std::isfinite(scale[axis]) || scale[axis] == 0.0f) {
                return false;
            }
            axes[axis] /= scale[axis];
        }

        if (std::abs(glm::dot(axes[0], axes[1])) > tolerance || std::abs(glm::dot(axes[0], axes[2])) > tolerance || std::abs(glm::dot(axes[1], axes[2])) > tolerance) {
            return false;
        }
        const float orientation = glm::dot(glm::cross(axes[0], axes[1]), axes[2]);
        if (!std::isfinite(orientation) || std::abs(std::abs(orientation) - 1.0f) > tolerance) {
            return false;
        }
        if (orientation < 0.0f) {
            axes[0] = -axes[0];
            scale.x = -scale.x;
        }

        const TransformValues candidate = {Vec3(matrix[3]), glm::quat_cast(Mat3(axes[0], axes[1], axes[2])), scale};
        const Mat4 reconstructed = transformMatrix(candidate);
        for (int column = 0; column < 4; column++) {
            for (int row = 0; row < 4; row++) {
                const float expected = matrix[column][row];
                const float actual = reconstructed[column][row];
                if (!std::isfinite(expected) || !std::isfinite(actual) || std::abs(expected - actual) > tolerance * std::max({1.0f, std::abs(expected), std::abs(actual)})) {
                    return false;
                }
            }
        }
        result = candidate;
        return true;
    }

    Scene::Scene(ComponentRegistry& registry) : m_registry(&registry), m_storage(std::make_unique<ArchetypeStorage>(registry)) {
    }

    Scene::~Scene() = default;

    Entity Scene::createEntity() {
        TE_PROFILER_FUNCTION();
        if (!immediateStructuralMutationAllowed()) {
            return {};
        }
        const Entity entity = m_storage->createEntity();
        getComponent<Transform>(entity).bind(*this, entity);
        return entity;
    }
    bool Scene::destroyEntity(Entity entity) {
        TE_PROFILER_FUNCTION();
        if (!immediateStructuralMutationAllowed()) {
            return false;
        }
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
        if (!immediateStructuralMutationAllowed()) {
            return;
        }
        m_storage->clear();
    }

    void* Scene::componentRaw(const Entity entity, const ComponentTypeId type) {
        return m_storage->componentRaw(entity, type);
    }

    const void* Scene::componentRaw(const Entity entity, const ComponentTypeId type) const {
        return m_storage->componentRaw(entity, type);
    }

    void* Scene::componentRawChecked(const Entity entity, const ComponentTypeId type) {
        void* component = componentRaw(entity, type);
        TE_CHECK(component != nullptr, "Entity has no requested component");
        return component;
    }

    const void* Scene::componentRawChecked(const Entity entity, const ComponentTypeId type) const {
        const void* component = componentRaw(entity, type);
        TE_CHECK(component != nullptr, "Entity has no requested component");
        return component;
    }

    bool Scene::fromLocalToWorld(Entity entity, Mat4& world) const {
        if (!hasComponent<Transform>(entity)) {
            return false;
        }
        Mat4 current(1.0f);
        for (; entity.valid(); entity = getParent(entity)) {
            if (!hasComponent<Transform>(entity)) {
                return false;
            }
            current = transformMatrix(getComponent<Transform>(entity).getLocal()) * current;
        }
        world = current;
        return true;
    }

    bool Scene::fromWorldToLocal(const Entity entity, const TransformValues& world, TransformValues& local) const {
        if (!hasComponent<Transform>(entity)) {
            return false;
        }
        Mat4 parentWorld(1.0f);
        const Entity parent = getParent(entity);
        if (parent.valid() && !fromLocalToWorld(parent, parentWorld)) {
            return false;
        }
        const float determinant = glm::determinant(Mat3(parentWorld));
        if (!std::isfinite(determinant) || determinant == 0.0f) {
            return false;
        }
        return decomposeLocalMatrix(glm::inverse(parentWorld) * transformMatrix(world), local);
    }

    void Scene::propagateTransforms() {
        for (const Entity root: getRoots()) {
            propagateTransformSubtree(root);
        }
    }

    void Scene::propagateTransformSubtree(const Entity root) {
        TE_PROFILER_FUNCTION();
        Entity entity = root;
        while (entity.valid()) {
            Transform& transform = getComponent<Transform>(entity);
            const Hierarchy* hierarchy = getHierarchy(entity);
            TE_CHECK(hierarchy != nullptr, "Entity has no hierarchy during transform propagation");

            Mat4 parentWorld(1.0f);
            TransformValues parentValues;
            if (hierarchy->m_parent.valid()) {
                const Transform& parentTransform = getComponent<Transform>(hierarchy->m_parent);
                parentWorld = parentTransform.worldMatrix();
                parentValues = parentTransform.getWorld();
            }
            transform.updateWorld(parentWorld, parentValues);

            if (hierarchy->m_firstChild.valid()) {
                entity = hierarchy->m_firstChild;
                continue;
            }

            while (entity != root) {
                const Hierarchy* current = getHierarchy(entity);
                TE_CHECK(current != nullptr, "Child became invalid during transform propagation");
                if (current->m_nextSibling.valid()) {
                    entity = current->m_nextSibling;
                    break;
                }
                entity = current->m_parent;
            }
            if (entity == root) {
                break;
            }
        }
    }

    Entity Scene::getParent(Entity entity) const {
        const Hierarchy* hierarchy = this->getHierarchy(entity);

        return hierarchy != nullptr ? hierarchy->m_parent : Entity{};
    }
    std::vector<Entity> Scene::getRoots() const {
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

        result.reserve(parentHierarchy->m_childrenCount);

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
        if (!immediateStructuralMutationAllowed()) {
            return false;
        }
        if (child == parent) {
            return false;
        }

        Hierarchy* childHierarchy = getHierarchy(child);
        Hierarchy* parentHierarchy = getHierarchy(parent);
        if (childHierarchy == nullptr || parentHierarchy == nullptr) {
            return false;
        }

        const Entity oldParent = childHierarchy->m_parent;
        if (oldParent != parent) {
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
        }

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
        if (oldParent != parent) {
            propagateTransformSubtree(child);
        }
        return true;
    }

    bool Scene::setParent(const Entity child, const Entity parent, const std::size_t position, const ReparentMode mode) {
        if (!immediateStructuralMutationAllowed()) {
            return false;
        }
        if (mode == ReparentMode::PreserveLocal || getParent(child) == parent) {
            return setParent(child, parent, position);
        }

        Mat4 childWorld;
        Mat4 parentWorld;
        if (!fromLocalToWorld(child, childWorld) || !fromLocalToWorld(parent, parentWorld)) {
            return false;
        }
        const float determinant = glm::determinant(Mat3(parentWorld));
        if (!std::isfinite(determinant) || determinant == 0.0f) {
            return false;
        }

        TransformValues newLocal;
        if (!decomposeLocalMatrix(glm::inverse(parentWorld) * childWorld, newLocal) || !setParent(child, parent, position)) {
            return false;
        }
        const bool updated = getComponent<Transform>(child).setLocal(newLocal);
        TE_CHECK(updated, "Validated local transform was rejected after reparenting");
        return true;
    }

    bool Scene::unparent(Entity child) {
        if (!immediateStructuralMutationAllowed()) {
            return false;
        }
        Hierarchy* childHierarchy = this->getHierarchy(child);
        if (childHierarchy == nullptr || !childHierarchy->m_parent.valid()) {
            return false;
        }

        Hierarchy* parentHierarchy = this->getHierarchy(childHierarchy->m_parent);
        Hierarchy* previousSiblingHierarchy = childHierarchy->m_previousSibling.valid() ? getHierarchy(childHierarchy->m_previousSibling) : nullptr;
        Hierarchy* nextSiblingHierarchy = childHierarchy->m_nextSibling.valid() ? getHierarchy(childHierarchy->m_nextSibling) : nullptr;
        if (parentHierarchy == nullptr || parentHierarchy->m_childrenCount == 0 || (childHierarchy->m_previousSibling.valid() && previousSiblingHierarchy == nullptr) || (childHierarchy->m_nextSibling.valid() && nextSiblingHierarchy == nullptr)) {
            return false;
        }
        if ((previousSiblingHierarchy != nullptr && previousSiblingHierarchy->m_nextSibling != child) || (previousSiblingHierarchy == nullptr && parentHierarchy->m_firstChild != child) || (nextSiblingHierarchy != nullptr && nextSiblingHierarchy->m_previousSibling != child)) {
            return false;
        }

        if (previousSiblingHierarchy != nullptr) {
            previousSiblingHierarchy->m_nextSibling = childHierarchy->m_nextSibling;
        } else {
            parentHierarchy->m_firstChild = childHierarchy->m_nextSibling;
        }
        if (nextSiblingHierarchy != nullptr) {
            nextSiblingHierarchy->m_previousSibling = childHierarchy->m_previousSibling;
        }
        parentHierarchy->m_childrenCount--;

        childHierarchy->m_parent = Entity();
        childHierarchy->m_previousSibling = Entity();
        childHierarchy->m_nextSibling = Entity();

        propagateTransformSubtree(child);
        return true;
    }

    bool Scene::unparent(const Entity child, const ReparentMode mode) {
        if (!immediateStructuralMutationAllowed()) {
            return false;
        }
        if (mode == ReparentMode::PreserveLocal) {
            return unparent(child);
        }

        if (!getParent(child).valid()) {
            return false;
        }
        Mat4 childWorld;
        TransformValues newLocal;
        if (!fromLocalToWorld(child, childWorld) || !decomposeLocalMatrix(childWorld, newLocal) || !unparent(child)) {
            return false;
        }
        const bool updated = getComponent<Transform>(child).setLocal(newLocal);
        TE_CHECK(updated, "Validated local transform was rejected after unparenting");
        return true;
    }

    bool Scene::reorderChild(Entity child, std::size_t position) {
        if (!immediateStructuralMutationAllowed()) {
            return false;
        }
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

    bool Scene::ownsTransform(const Entity entity, const Transform* transform) const {
        return m_storage->component<Transform>(entity) == transform;
    }

    SceneCommandBuffer& Scene::getCommands() {
        TE_CHECK(g_sceneExecutionState.scene == this && g_sceneExecutionState.commands != nullptr, "Scene commands are available only while a system is executing");
        return *g_sceneExecutionState.commands;
    }

    bool Scene::isSystemExecuting() const {
        return g_sceneExecutionState.scene == this;
    }

    bool Scene::immediateStructuralMutationAllowed() const {
        if (!isSystemExecuting()) {
            return true;
        }
        TE_CHECK(false, "Immediate structural mutation is prohibited while a system is executing");
        return false;
    }

    void Scene::validateRead(const ComponentTypeId type) const {
        if (g_sceneExecutionState.scene != this) {
            return;
        }
        const ComponentTypeRecord* record = m_registry->find(type);
        TE_ASSERT(record != nullptr && g_sceneExecutionState.access->reads(record->denseId), "System read an undeclared component");
    }

    void Scene::validateWrite(const ComponentTypeId type) {
        if (g_sceneExecutionState.scene != this) {
            return;
        }
        const ComponentTypeRecord* record = m_registry->find(type);
        TE_ASSERT(record != nullptr && g_sceneExecutionState.access->writes(record->denseId), "System wrote an undeclared component");
    }

    std::uint64_t Scene::getChangeTickRaw(const Entity entity, const ComponentTypeId type) const {
        const ComponentTypeRecord* record = m_registry->find(type);
        if (record == nullptr) {
            return 0;
        }
        return m_storage->getChangeTick(entity, record->denseId);
    }

    void Scene::beginSystem(const ScheduleAccess& access, SceneCommandBuffer& commands, const std::uint64_t tick) {
        TE_CHECK(g_sceneExecutionState.scene == nullptr, "A system execution scope is already active on this thread");
        g_sceneExecutionState = {this, &access, &commands};
        access.forEachWrittenType([this, tick](const ComponentDenseId type) {
            m_storage->markChanged(type, tick);
        });
    }

    void Scene::endSystem() {
        TE_CHECK(g_sceneExecutionState.scene == this, "No system execution scope is active for this Scene");
        g_sceneExecutionState = {};
    }

    void Scene::applyCommands(SceneCommandBuffer& commands, std::vector<Entity>& spawned) {
        TE_CHECK(g_sceneExecutionState.scene == nullptr, "Cannot apply structural commands while a system is executing");
        commands.apply(*this, spawned);
    }

    bool Scene::addComponentInternal(const Entity entity, const ComponentTypeId type, const void* value) const {
        const ComponentTypeRecord* record = m_registry->find(type);
        TE_CHECK(record != nullptr, "Component type is not registered");
        return m_storage->addComponent(entity, record->denseId, value);
    }

    bool Scene::removeComponentInternal(const Entity entity, const ComponentTypeId type) const {
        const ComponentTypeRecord* record = m_registry->find(type);
        TE_CHECK(record != nullptr, "Component type is not registered");
        return m_storage->removeComponent(entity, record->denseId);
    }
}
