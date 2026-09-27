#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/core/events/EventStream.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <utility>

namespace TechEngine {
    EventStream::EventStream(const EventTypeId id, const std::uint32_t elementSize, const std::uint32_t alignment, const std::size_t initialCapacity) : m_id(id), m_elementSize(elementSize), m_alignment(alignment) {
        TE_CHECK(m_elementSize > 0);
        TE_CHECK(m_alignment <= alignof(std::max_align_t));

        m_visible.storage = std::make_unique<std::byte[]>(initialCapacity * m_elementSize);
        m_visible.capacity = initialCapacity;
        m_staging.storage = std::make_unique<std::byte[]>(initialCapacity * m_elementSize);
        m_staging.capacity = initialCapacity;
    }

    void EventStream::makeVisible(const std::uint64_t tick) {
        if (!TE_VERIFY(m_visible.count == 0, "Event type {0} made Tick {1} visible before retiring Tick {2}", m_id.stringId().value(), tick, m_visibleTick)) {
            return;
        }

        std::swap(m_visible, m_staging);
        m_visibleTick = tick;
    }

    void EventStream::retire() {
        m_visible.count = 0;
    }

    EventTypeId EventStream::id() const {
        return m_id;
    }

    std::size_t EventStream::visibleCount() const {
        return m_visible.count;
    }

    std::size_t EventStream::stagedCount() const {
        return m_staging.count;
    }

    std::size_t EventStream::capacity() const {
        return m_staging.capacity;
    }

    std::uint64_t EventStream::visibleTick() const {
        return m_visibleTick;
    }

    void EventStream::stage(const void* event) {
        if (m_staging.count == m_staging.capacity) {
            grow(m_staging.capacity * 2);
        }

        std::memcpy(m_staging.storage.get() + m_staging.count * m_elementSize, event, m_elementSize);
        m_staging.count++;
    }

    EventStream::ByteRange EventStream::visibleRange() const {
        return ByteRange{m_visible.storage.get(), m_visible.count};
    }

    void EventStream::grow(std::size_t minimumCapacity) {
        const std::size_t newCapacity = std::max<std::size_t>(minimumCapacity, 1);
        std::unique_ptr<std::byte[]> storage = std::make_unique<std::byte[]>(newCapacity * m_elementSize);

        std::memcpy(storage.get(), m_staging.storage.get(), m_staging.count * m_elementSize);

        m_staging.storage = std::move(storage);
        m_staging.capacity = newCapacity;
    }
}
