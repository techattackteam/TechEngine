#pragma once

#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/core/events/EventTypeId.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace TechEngine {
    class EventStream {
    private:
        struct Buffer {
            std::unique_ptr<std::byte[]> storage;
            std::size_t capacity = 0;
            std::size_t count = 0;
        };

        EventTypeId m_id;
        std::uint32_t m_elementSize = 0;
        Buffer m_visible;
        Buffer m_staging;
        std::uint64_t m_visibleTick = 0;

    public:
        EventStream(EventTypeId id, std::uint32_t elementSize, std::uint32_t alignment, std::size_t initialCapacity);

        EventStream(const EventStream&) = delete;

        EventStream& operator=(const EventStream&) = delete;

        EventStream(EventStream&&) noexcept = default;

        EventStream& operator=(EventStream&&) noexcept = default;

        template<typename T>
        void publish(const T& event) {
            TE_ASSERT(m_id == eventTypeId<T>());
            stage(&event);
        }

        template<typename T>
        std::span<const T> read() const {
            TE_ASSERT(m_id == eventTypeId<T>());
            return std::span<const T>{reinterpret_cast<const T*>(m_visible.storage.get()), m_visible.count};
        }

        void makeVisible(std::uint64_t tick);

        void retire();

        std::size_t visibleCount() const;

        std::size_t stagedCount() const;

        std::size_t capacity() const;

        std::uint64_t visibleTick() const;

    private:
        Buffer makeBuffer(std::size_t capacity) const;

        void stage(const void* event);

        void grow();
    };
}
