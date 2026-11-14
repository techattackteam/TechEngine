#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneCommandBuffer.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Name.hpp>
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

    static float frobeniusNorm(const Mat3& matrix) {
        return std::sqrt(glm::dot(matrix[0], matrix[0]) + glm::dot(matrix[1], matrix[1]) + glm::dot(matrix[2], matrix[2]));
    }

    static bool isFiniteVector(const Vec3& value) {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    }

    static bool approximateLocalMatrix(const Mat4& matrix, TransformValues& result) {
        constexpr int POLAR_ITERATIONS = 8;
        constexpr float MINIMUM_RELATIVE_VOLUME = 0.00001f;
        Mat3 linear = Mat3(matrix);
        const Vec3 lengths = Vec3(glm::length(linear[0]), glm::length(linear[1]), glm::length(linear[2]));
        // A matrix flattened by a zero scale axis rounds to a tiny nonzero determinant, not to zero.
        const float relativeVolume = glm::determinant(Mat3(linear[0] / lengths.x, linear[1] / lengths.y, linear[2] / lengths.z));
        if (!std::isfinite(relativeVolume) || std::abs(relativeVolume) <= MINIMUM_RELATIVE_VOLUME) {
            return false;
        }
        const bool mirrored = relativeVolume < 0.0f;
        if (mirrored) {
            linear[0] = -linear[0];
        }

        // A singular step yields infinities or NaNs that carry through to the result, which is checked once below.
        Mat3 polar = linear / std::max({lengths.x, lengths.y, lengths.z});
        for (int iteration = 0; iteration < POLAR_ITERATIONS; iteration++) {
            const Mat3 inverted = glm::inverse(polar);
            const float normScale = std::sqrt(frobeniusNorm(inverted) / frobeniusNorm(polar));
            polar = 0.5f * (normScale * polar + glm::transpose(inverted) / normScale);
        }

        Vec3 scale = Vec3(glm::dot(polar[0], linear[0]), glm::dot(polar[1], linear[1]), glm::dot(polar[2], linear[2]));
        if (mirrored) {
            scale.x = -scale.x;
        }
        const Vec3 position = Vec3(matrix[3]);
        const Quat rotation = glm::normalize(glm::quat_cast(polar));
        const bool finite = isFiniteVector(position) && isFiniteVector(scale) && isFiniteVector(Vec3(rotation.x, rotation.y, rotation.z)) && std::isfinite(rotation.w);
        if (!finite || glm::any(glm::equal(scale, Vec3(0.0f)))) {
            return false;
        }
        result = {position, rotation, scale};
        return true;
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

        const float orientation = glm::dot(glm::cross(axes[0], axes[1]), axes[2]);
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
                    return approximateLocalMatrix(matrix, result);
                }
            }
        }
        result = candidate;
        return true;
    }

    Scene::Scene(ComponentRegistry& registry) : m_registry(&registry), m_storage(std::make_unique<ArchetypeStorage>(registry)), m_querySource(m_storage.get()) {
    }

    Scene::~Scene() = default;

    Entity Scene::createEntity() {
        TE_PROFILER_FUNCTION();
        if (!immediateStructuralMutationAllowed()) {
            return {};
        }
        const Entity entity = m_storage->createEntity();
        getComponent<Transform>(entity).bind(*this, entity);
        linkChild(entity, *getHierarchy(entity), Entity(), m_roots.m_lastChild, Entity());
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

        if (!hasConsistentLinks(entity, *rootHierarchy)) {
            return false;
        }
        unlinkChild(*rootHierarchy);

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
        m_roots = Hierarchy();
    }

    void* Scene::componentRaw(const Entity entity, const ComponentTypeId type) {
        return m_storage->componentRaw(entity, type);
    }

    const void* Scene::componentRaw(const Entity entity, const ComponentTypeId type) const {
        return m_storage->componentRaw(entity, type);
    }

    bool Scene::addComponentRaw(const Entity entity, const ComponentTypeId type) {
        const ComponentTypeRecord* record = m_registry->find(type);
        if (record == nullptr || isBuiltInComponent(type)) {
            return false;
        }

        const std::unique_ptr<IComponentStorage> scratch = record->createStorage();
        scratch->appendDefault();
        return addComponentInternal(entity, type, scratch->element(0));
    }

    bool Scene::removeComponentRaw(const Entity entity, const ComponentTypeId type) {
        if (m_registry->find(type) == nullptr || isBuiltInComponent(type)) {
            return false;
        }

        return removeComponentInternal(entity, type);
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
        return collectChildren(m_roots);
    }

    std::vector<Entity> Scene::getChildren(Entity parent) const {
        TE_PROFILER_FUNCTION();
        const Hierarchy* parentHierarchy = this->getHierarchy(parent);
        if (parentHierarchy == nullptr) {
            return {};
        }
        return collectChildren(*parentHierarchy);
    }

    bool Scene::setParent(const Entity child, const Entity parent, const std::size_t position) {
        TE_PROFILER_FUNCTION();
        if (!immediateStructuralMutationAllowed()) {
            return false;
        }
        if (child == parent || getHierarchy(parent) == nullptr) {
            return false;
        }
        return moveChild(child, parent, position);
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

    bool Scene::unparent(const Entity child) {
        return unparent(child, m_roots.m_childrenCount, ReparentMode::PreserveLocal);
    }

    bool Scene::unparent(const Entity child, const ReparentMode mode) {
        return unparent(child, m_roots.m_childrenCount, mode);
    }

    bool Scene::unparent(const Entity child, const std::size_t position, const ReparentMode mode) {
        if (!immediateStructuralMutationAllowed()) {
            return false;
        }
        if (!getParent(child).valid()) {
            return false;
        }
        if (mode == ReparentMode::PreserveLocal) {
            return moveChild(child, Entity(), position);
        }

        Mat4 childWorld;
        TransformValues newLocal;
        if (!fromLocalToWorld(child, childWorld) || !decomposeLocalMatrix(childWorld, newLocal) || !moveChild(child, Entity(), position)) {
            return false;
        }
        const bool updated = getComponent<Transform>(child).setLocal(newLocal);
        TE_CHECK(updated, "Validated local transform was rejected after unparenting");
        return true;
    }

    bool Scene::reorderChild(const Entity child, const std::size_t position) {
        if (!immediateStructuralMutationAllowed()) {
            return false;
        }
        const Hierarchy* childHierarchy = getHierarchy(child);
        if (childHierarchy == nullptr) {
            return false;
        }
        return moveChild(child, childHierarchy->m_parent, position);
    }

    Hierarchy* Scene::getHierarchy(const Entity entity) {
        return m_storage->component<Hierarchy>(entity);
    }

    const Hierarchy* Scene::getHierarchy(const Entity entity) const {
        return m_storage->component<Hierarchy>(entity);
    }

    Hierarchy* Scene::getChildList(const Entity parent) {
        return parent.valid() ? getHierarchy(parent) : &m_roots;
    }

    const Hierarchy* Scene::getChildList(const Entity parent) const {
        return parent.valid() ? getHierarchy(parent) : &m_roots;
    }

    std::vector<Entity> Scene::collectChildren(const Hierarchy& list) const {
        std::vector<Entity> result;
        result.reserve(list.m_childrenCount);
        Entity current = list.m_firstChild;
        while (current.valid()) {
            result.push_back(current);
            const Hierarchy* currentHierarchy = getHierarchy(current);
            if (currentHierarchy == nullptr) {
                break;
            }
            current = currentHierarchy->m_nextSibling;
        }
        return result;
    }

    bool Scene::hasConsistentLinks(const Entity child, const Hierarchy& childHierarchy) const {
        const Entity previous = childHierarchy.m_previousSibling;
        const Entity next = childHierarchy.m_nextSibling;
        const Hierarchy* list = getChildList(childHierarchy.m_parent);
        const Hierarchy* previousHierarchy = previous.valid() ? getHierarchy(previous) : nullptr;
        const Hierarchy* nextHierarchy = next.valid() ? getHierarchy(next) : nullptr;
        if (list == nullptr || list->m_childrenCount == 0 || (previous.valid() && previousHierarchy == nullptr) || (next.valid() && nextHierarchy == nullptr)) {
            return false;
        }
        const Entity linkedFromPrevious = previousHierarchy != nullptr ? previousHierarchy->m_nextSibling : list->m_firstChild;
        const Entity linkedFromNext = nextHierarchy != nullptr ? nextHierarchy->m_previousSibling : list->m_lastChild;
        return linkedFromPrevious == child && linkedFromNext == child;
    }

    void Scene::unlinkChild(Hierarchy& childHierarchy) {
        Hierarchy* list = getChildList(childHierarchy.m_parent);
        Hierarchy* previousHierarchy = childHierarchy.m_previousSibling.valid() ? getHierarchy(childHierarchy.m_previousSibling) : nullptr;
        Hierarchy* nextHierarchy = childHierarchy.m_nextSibling.valid() ? getHierarchy(childHierarchy.m_nextSibling) : nullptr;
        TE_CHECK(list != nullptr && (!childHierarchy.m_previousSibling.valid() || previousHierarchy != nullptr) && (!childHierarchy.m_nextSibling.valid() || nextHierarchy != nullptr), "Unlinked a child whose sibling links were not checked");

        if (previousHierarchy != nullptr) {
            previousHierarchy->m_nextSibling = childHierarchy.m_nextSibling;
        } else {
            list->m_firstChild = childHierarchy.m_nextSibling;
        }
        if (nextHierarchy != nullptr) {
            nextHierarchy->m_previousSibling = childHierarchy.m_previousSibling;
        } else {
            list->m_lastChild = childHierarchy.m_previousSibling;
        }
        list->m_childrenCount--;

        childHierarchy.m_parent = Entity();
        childHierarchy.m_previousSibling = Entity();
        childHierarchy.m_nextSibling = Entity();
    }

    void Scene::linkChild(const Entity child, Hierarchy& childHierarchy, const Entity parent, const Entity before, const Entity after) {
        Hierarchy* list = getChildList(parent);
        Hierarchy* beforeHierarchy = before.valid() ? getHierarchy(before) : nullptr;
        Hierarchy* afterHierarchy = after.valid() ? getHierarchy(after) : nullptr;
        TE_CHECK(list != nullptr && (!before.valid() || beforeHierarchy != nullptr) && (!after.valid() || afterHierarchy != nullptr), "Linked a child beside an entity that has no hierarchy");

        childHierarchy.m_parent = parent;
        childHierarchy.m_previousSibling = before;
        childHierarchy.m_nextSibling = after;
        if (beforeHierarchy != nullptr) {
            beforeHierarchy->m_nextSibling = child;
        } else {
            list->m_firstChild = child;
        }
        if (afterHierarchy != nullptr) {
            afterHierarchy->m_previousSibling = child;
        } else {
            list->m_lastChild = child;
        }
        list->m_childrenCount++;
    }

    bool Scene::moveChild(const Entity child, const Entity parent, const std::size_t position) {
        Hierarchy* childHierarchy = getHierarchy(child);
        const Hierarchy* destination = getChildList(parent);
        if (childHierarchy == nullptr || destination == nullptr) {
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
        if (!hasConsistentLinks(child, *childHierarchy)) {
            return false;
        }

        const std::size_t destinationCount = destination->m_childrenCount - static_cast<std::size_t>(oldParent == parent);
        if (position > destinationCount) {
            return false;
        }

        Entity insertionBefore;
        Entity insertionAfter;
        if (position == destinationCount) {
            insertionBefore = destination->m_lastChild == child ? childHierarchy->m_previousSibling : destination->m_lastChild;
        } else {
            std::size_t visited = 0;
            for (Entity current = destination->m_firstChild; current.valid();) {
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
            if (!insertionAfter.valid()) {
                return false;
            }
        }
        if ((insertionBefore.valid() && getHierarchy(insertionBefore) == nullptr) || (insertionAfter.valid() && getHierarchy(insertionAfter) == nullptr)) {
            return false;
        }

        unlinkChild(*childHierarchy);
        linkChild(child, *childHierarchy, parent, insertionBefore, insertionAfter);
        if (oldParent != parent) {
            propagateTransformSubtree(child);
        }
        return true;
    }

    bool Scene::ownsTransform(const Entity entity, const Transform* transform) const {
        return m_storage->component<Transform>(entity) == transform;
    }

    SceneCommandBuffer& Scene::getCommands() {
        TE_CHECK(g_sceneExecutionState.scene == this && g_sceneExecutionState.commands != nullptr, "Scene commands are available only while a system is executing");
        return *g_sceneExecutionState.commands;
    }

    void Scene::buildEventStreams(EventRegistry& registry) {
        if (!TE_VERIFY(!m_events.has_value(), "Scene event streams are already built")) {
            return;
        }
        m_events.emplace(registry);
    }

    void Scene::makeEventsVisible(const std::uint64_t tick) {
        if (!eventBarrierAllowed()) {
            return;
        }
        m_events->makeVisible(tick);
    }

    void Scene::retireEvents() {
        if (!eventBarrierAllowed()) {
            return;
        }
        m_events->retire();
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

    bool Scene::eventAccessAllowed() const {
        if (!isSystemExecuting()) {
            TE_CHECK(false, "Scene events are available only while a system is executing");
            return false;
        }
        return TE_VERIFY(m_events.has_value(), "Scene events were used before the Scene's streams were built");
    }

    bool Scene::eventBarrierAllowed() const {
        if (isSystemExecuting()) {
            TE_CHECK(false, "Scene events cannot be made visible or retired while a system is executing");
            return false;
        }
        return m_events.has_value();
    }

    std::span<const std::byte> Scene::readEventBytes(const EventTypeId type) const {
        if (!TE_VERIFY(m_events.has_value(), "Scene events were used before the Scene's streams were built")) {
            return {};
        }
        return m_events->readBytes(type);
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
        return m_storage->addComponent(entity, m_registry->getDenseId(type), value);
    }

    bool Scene::removeComponentInternal(const Entity entity, const ComponentTypeId type) const {
        return m_storage->removeComponent(entity, m_registry->getDenseId(type));
    }

    bool Scene::isBuiltInComponent(const ComponentTypeId type) const {
        return type == componentTypeId<Hierarchy>() || type == componentTypeId<Transform>() || type == componentTypeId<Name>();
    }
}
