#pragma once

#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Query.hpp>
#include <TechEngine/core/scene/SceneCommandBuffer.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace TechEngine {
    class ArchetypeStorage;
    class ComponentRegistry;
    class Hierarchy;
    class ScheduleAccess;
    class Transform;
    struct TransformValues;

    enum class ReparentMode { PreserveLocal, PreserveWorld };

    class Scene {
    private:
        friend class Transform;
        friend class SceneCommandBuffer;
        friend class SerialExecutor;

        ComponentRegistry* m_registry = nullptr;
        std::unique_ptr<ArchetypeStorage> m_storage;
        internal::QuerySource* m_querySource = nullptr;

    public:
        explicit Scene(ComponentRegistry& registry);

        ~Scene();

        Scene(const Scene&) = delete;

        Scene(Scene&&) = delete;

        Scene& operator=(const Scene&) = delete;

        Scene& operator=(Scene&&) = delete;

        Entity createEntity();

        bool destroyEntity(Entity entity);

        bool contains(Entity entity) const;

        void clear();

        template<typename Component>
            requires(!std::same_as<Component, Hierarchy>)
        Component& getComponent(const Entity entity) {
            validateWrite(componentTypeId<Component>());
            return *static_cast<Component*>(componentRawChecked(entity, componentTypeId<Component>()));
        }

        template<typename Component>
        const Component& getComponent(const Entity entity) const {
            validateRead(componentTypeId<Component>());
            return *static_cast<const Component*>(componentRawChecked(entity, componentTypeId<Component>()));
        }

        template<typename Component>
        bool hasComponent(const Entity entity) const {
            validateRead(componentTypeId<Component>());
            return componentRaw(entity, componentTypeId<Component>()) != nullptr;
        }

        template<ComponentValue Component, typename... Arguments>
            requires(!std::same_as<Component, Hierarchy>) && (!std::same_as<Component, Transform>) && std::constructible_from<Component, Arguments...>
        void addComponent(const Entity entity, Arguments&&... arguments) {
            if (isSystemExecuting()) {
                getCommands().addComponent<Component>(entity, std::forward<Arguments>(arguments)...);
                return;
            }
            Component value = Component(std::forward<Arguments>(arguments)...);
            (void)addComponentInternal(entity, componentTypeId<Component>(), &value);
        }

        // Pending targets preserve barrier order. Redesign spawn to reserve an Entity if this
        // becomes a user-facing problem.
        template<ComponentValue Component, typename... Arguments>
            requires(!std::same_as<Component, Hierarchy>) && (!std::same_as<Component, Transform>) && std::constructible_from<Component, Arguments...>
        void addComponent(const PendingEntity entity, Arguments&&... arguments) {
            getCommands().addComponent<Component>(entity, std::forward<Arguments>(arguments)...);
        }

        template<ComponentValue Component>
            requires(!std::same_as<Component, Hierarchy>) && (!std::same_as<Component, Transform>)
        void removeComponent(const Entity entity) {
            if (isSystemExecuting()) {
                getCommands().removeComponent<Component>(entity);
                return;
            }
            (void)removeComponentInternal(entity, componentTypeId<Component>());
        }

        template<ComponentValue Component>
            requires(!std::same_as<Component, Hierarchy>)
        void removeComponent(const PendingEntity entity) {
            getCommands().removeComponent<Component>(entity);
        }

        template<typename Component>
        std::uint64_t getChangeTick(const Entity entity) const {
            return getChangeTickRaw(entity, componentTypeId<Component>());
        }

        template<typename WritableComponents, typename ReadableComponents>
        Query<WritableComponents, ReadableComponents> query() {
            using QueryType = Query<WritableComponents, ReadableComponents>;
            QueryType::forEachWrittenType([this](const ComponentTypeId type) {
                validateWrite(type);
            });
            QueryType::forEachReadOnlyType([this](const ComponentTypeId type) {
                validateRead(type);
            });
            return QueryType(*m_querySource);
        }

        SceneCommandBuffer& getCommands();

        bool fromLocalToWorld(Entity entity, Mat4& world) const;

        bool fromWorldToLocal(Entity entity, const TransformValues& world, TransformValues& local) const;

        void propagateTransforms();

        Entity getParent(Entity entity) const;

        std::vector<Entity> getRoots() const;

        std::vector<Entity> getChildren(Entity parent) const;

        bool setParent(Entity child, Entity parent, std::size_t position = 0);

        bool setParent(Entity child, Entity parent, std::size_t position, ReparentMode mode);

        bool unparent(Entity child);

        bool unparent(Entity child, ReparentMode mode);

        bool reorderChild(Entity child, std::size_t position);

    private:
        void* componentRaw(Entity entity, ComponentTypeId type);

        const void* componentRaw(Entity entity, ComponentTypeId type) const;

        void* componentRawChecked(Entity entity, ComponentTypeId type);

        const void* componentRawChecked(Entity entity, ComponentTypeId type) const;

        Hierarchy* getHierarchy(Entity entity);

        const Hierarchy* getHierarchy(Entity entity) const;

        bool ownsTransform(Entity entity, const Transform* transform) const;

        void propagateTransformSubtree(Entity root);

        bool isSystemExecuting() const;

        bool immediateStructuralMutationAllowed() const;

        void validateRead(ComponentTypeId type) const;

        void validateWrite(ComponentTypeId type);

        std::uint64_t getChangeTickRaw(Entity entity, ComponentTypeId type) const;

        void beginSystem(const ScheduleAccess& access, SceneCommandBuffer& commands, std::uint64_t tick);

        void endSystem();

        void applyCommands(SceneCommandBuffer& commands, std::vector<Entity>& spawned);

        bool addComponentInternal(Entity entity, ComponentTypeId type, const void* value) const;

        bool removeComponentInternal(Entity entity, ComponentTypeId type) const;
    };
}
