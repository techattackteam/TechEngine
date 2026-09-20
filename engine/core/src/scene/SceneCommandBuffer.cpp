#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneCommandBuffer.hpp>

#include <atomic>
#include <limits>
#include <new>
#include <utility>
#include <variant>
#include <vector>

namespace TechEngine {
    enum class SceneCommandKind { Spawn, Despawn, AddComponent, RemoveComponent };

    using SceneCommandTarget = std::variant<Entity, PendingEntity>;

    static std::atomic<std::uint64_t> g_nextSceneCommandBufferId = 1;

    SceneCommandBuffer::Payload::Payload(const void* source, const Operations operations) : m_operations(operations) {
        TE_CHECK(m_operations.size > 0 && m_operations.alignment > 0 && m_operations.copyConstruct != nullptr && m_operations.destroy != nullptr, "Component value operations are incomplete");
        m_value = ::operator new(m_operations.size, std::align_val_t(m_operations.alignment));
        try {
            m_operations.copyConstruct(m_value, source);
        } catch (...) {
            ::operator delete(m_value, std::align_val_t(m_operations.alignment));
            m_value = nullptr;
            throw;
        }
    }

    SceneCommandBuffer::Payload::~Payload() {
        reset();
    }

    void SceneCommandBuffer::Payload::reset() noexcept {
        if (m_value == nullptr) {
            return;
        }
        m_operations.destroy(m_value);
        ::operator delete(m_value, std::align_val_t(m_operations.alignment));
        m_value = nullptr;
    }

    SceneCommandBuffer::Payload::Payload(Payload&& other) noexcept : m_value(std::exchange(other.m_value, nullptr)), m_operations(other.m_operations) {
    }

    SceneCommandBuffer::Payload& SceneCommandBuffer::Payload::operator=(Payload&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        reset();
        m_value = std::exchange(other.m_value, nullptr);
        m_operations = other.m_operations;
        return *this;
    }

    const void* SceneCommandBuffer::Payload::getValue() const {
        return m_value;
    }

    class SceneCommandBuffer::Impl {
    public:
        struct Command {
            SceneCommandKind kind = SceneCommandKind::Spawn;
            SceneCommandTarget target;
            ComponentTypeId component;
            Payload payload;
        };

        std::uint64_t id = g_nextSceneCommandBufferId.fetch_add(1, std::memory_order_relaxed);
        std::uint64_t epoch = 1;
        std::uint32_t nextPendingIndex = 0;
        std::vector<Command> commands;
        std::vector<Entity> resolvedEntities;
    };

    PendingEntity::PendingEntity(const std::uint64_t bufferId, const std::uint64_t epoch, const std::uint32_t index) : m_bufferId(bufferId), m_epoch(epoch), m_index(index) {
    }

    bool PendingEntity::valid() const {
        return m_bufferId != 0;
    }

    SceneCommandBuffer::SceneCommandBuffer() : m_impl(std::make_unique<Impl>()) {
    }

    SceneCommandBuffer::~SceneCommandBuffer() = default;

    SceneCommandBuffer::SceneCommandBuffer(SceneCommandBuffer&&) noexcept = default;

    SceneCommandBuffer& SceneCommandBuffer::operator=(SceneCommandBuffer&&) noexcept = default;

    PendingEntity SceneCommandBuffer::spawn() const {
        TE_CHECK(m_impl->nextPendingIndex != std::numeric_limits<std::uint32_t>::max(), "Pending entity index exhausted");
        const PendingEntity pending(m_impl->id, m_impl->epoch, m_impl->nextPendingIndex++);
        m_impl->commands.push_back({SceneCommandKind::Spawn, pending, {}, Payload{}});
        return pending;
    }

    void SceneCommandBuffer::despawn(const Entity entity) const {
        m_impl->commands.push_back({SceneCommandKind::Despawn, entity, {}, Payload{}});
    }

    void SceneCommandBuffer::despawn(const PendingEntity entity) const {
        TE_CHECK(entity.m_bufferId == m_impl->id && entity.m_epoch == m_impl->epoch, "Pending entity belongs to a different command buffer execution");
        m_impl->commands.push_back({SceneCommandKind::Despawn, entity, {}, Payload{}});
    }

    void SceneCommandBuffer::queueAdd(const Entity entity, const ComponentTypeId type, Payload payload) const {
        m_impl->commands.push_back({SceneCommandKind::AddComponent, entity, type, std::move(payload)});
    }

    void SceneCommandBuffer::queueAdd(const PendingEntity entity, const ComponentTypeId type, Payload payload) const {
        TE_CHECK(entity.m_bufferId == m_impl->id && entity.m_epoch == m_impl->epoch, "Pending entity belongs to a different command buffer execution");
        m_impl->commands.push_back({SceneCommandKind::AddComponent, entity, type, std::move(payload)});
    }

    void SceneCommandBuffer::queueRemove(const Entity entity, const ComponentTypeId type) const {
        m_impl->commands.push_back({SceneCommandKind::RemoveComponent, entity, type, Payload{}});
    }

    void SceneCommandBuffer::queueRemove(const PendingEntity entity, const ComponentTypeId type) const {
        TE_CHECK(entity.m_bufferId == m_impl->id && entity.m_epoch == m_impl->epoch, "Pending entity belongs to a different command buffer execution");
        m_impl->commands.push_back({SceneCommandKind::RemoveComponent, entity, type, Payload{}});
    }

    void SceneCommandBuffer::apply(Scene& scene, std::vector<Entity>& spawned) const {
        m_impl->resolvedEntities.clear();
        const auto resolveTarget = [this](const SceneCommandTarget& target) {
            if (std::holds_alternative<Entity>(target)) {
                return std::get<Entity>(target);
            }
            const PendingEntity pending = std::get<PendingEntity>(target);
            TE_CHECK(pending.m_bufferId == m_impl->id && pending.m_epoch == m_impl->epoch, "Pending entity belongs to a different command buffer execution");
            TE_CHECK(pending.m_index < m_impl->resolvedEntities.size(), "Pending entity has not been spawned yet");
            return m_impl->resolvedEntities[pending.m_index];
        };

        for (const Impl::Command& command: m_impl->commands) {
            switch (command.kind) {
                case SceneCommandKind::Spawn: {
                    const Entity entity = scene.createEntity();
                    m_impl->resolvedEntities.push_back(entity);
                    spawned.push_back(entity);
                    break;
                }
                case SceneCommandKind::Despawn:
                    scene.destroyEntity(resolveTarget(command.target));
                    break;
                case SceneCommandKind::AddComponent:
                    scene.addComponentInternal(resolveTarget(command.target), command.component, command.payload.getValue());
                    break;
                case SceneCommandKind::RemoveComponent:
                    scene.removeComponentInternal(resolveTarget(command.target), command.component);
                    break;
            }
        }
        discard();
    }

    void SceneCommandBuffer::discard() const {
        m_impl->commands.clear();
        m_impl->resolvedEntities.clear();
        m_impl->nextPendingIndex = 0;
        m_impl->epoch++;
        TE_CHECK(m_impl->epoch != 0, "Scene command buffer epoch exhausted");
    }
}
