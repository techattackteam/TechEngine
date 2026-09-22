#pragma once

#include <TechEngine/core/scene/ComponentStorage.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace TechEngine {
    class Scene;

    struct PendingEntity {
    private:
        std::uint64_t m_bufferId = 0;
        std::uint64_t m_epoch = 0;
        std::uint32_t m_index = 0;

        PendingEntity(std::uint64_t bufferId, std::uint64_t epoch, std::uint32_t index);

        friend class SceneCommandBuffer;

    public:
        PendingEntity() = default;

        bool valid() const;

        bool operator==(const PendingEntity&) const = default;
    };

    class SceneCommandBuffer {
    private:
        class Payload {
        private:
            struct Operations {
                std::size_t size = 0;
                std::size_t alignment = 0;
                void (*copyConstruct)(void* destination, const void* source) = nullptr;
                void (*destroy)(void* value) noexcept = nullptr;
            };

            void* m_value = nullptr;
            Operations m_operations;

            Payload(const void* source, Operations operations);

            void reset() noexcept;

        public:
            Payload() = default;

            ~Payload();

            Payload(const Payload&) = delete;

            Payload(Payload&& other) noexcept;

            Payload& operator=(const Payload&) = delete;

            Payload& operator=(Payload&& other) noexcept;

            template<ComponentValue Component>
            static Payload copyOf(const Component& value) {
                const Operations operations{
                    sizeof(Component),
                    alignof(Component),
                    [](void* destination, const void* source) {
                        std::construct_at(static_cast<Component*>(destination), *static_cast<const Component*>(source));
                    },
                    [](void* storedValue) noexcept {
                        std::destroy_at(static_cast<Component*>(storedValue));
                    },
                };
                return Payload(&value, operations);
            }

            const void* getValue() const;
        };

        class Impl;

        std::unique_ptr<Impl> m_impl;

    public:
        SceneCommandBuffer();

        ~SceneCommandBuffer();

        SceneCommandBuffer(const SceneCommandBuffer&) = delete;

        SceneCommandBuffer(SceneCommandBuffer&&) noexcept;

        SceneCommandBuffer& operator=(const SceneCommandBuffer&) = delete;

        SceneCommandBuffer& operator=(SceneCommandBuffer&&) noexcept;

        PendingEntity spawn() const;

        void despawn(Entity entity) const;

        void despawn(PendingEntity entity) const;

    private:
        template<ComponentValue Component, typename... Arguments>
            requires std::constructible_from<Component, Arguments...>
        void addComponent(const Entity entity, Arguments&&... arguments) {
            Component value = Component(std::forward<Arguments>(arguments)...);
            queueAdd(entity, componentTypeId<Component>(), Payload::copyOf(value));
        }

        template<ComponentValue Component, typename... Arguments>
            requires std::constructible_from<Component, Arguments...>
        void addComponent(const PendingEntity entity, Arguments&&... arguments) {
            Component value = Component(std::forward<Arguments>(arguments)...);
            queueAdd(entity, componentTypeId<Component>(), Payload::copyOf(value));
        }

        template<ComponentValue Component>
        void removeComponent(const Entity entity) {
            queueRemove(entity, componentTypeId<Component>());
        }

        template<ComponentValue Component>
        void removeComponent(const PendingEntity entity) {
            queueRemove(entity, componentTypeId<Component>());
        }

        void queueAdd(Entity entity, ComponentTypeId type, Payload payload) const;

        void queueAdd(PendingEntity entity, ComponentTypeId type, Payload payload) const;

        void queueRemove(Entity entity, ComponentTypeId type) const;

        void queueRemove(PendingEntity entity, ComponentTypeId type) const;

        void apply(Scene& scene, std::vector<Entity>& spawned) const;

        void discard() const;

        friend class Scene;
        friend class SerialExecutor;
    };
}
