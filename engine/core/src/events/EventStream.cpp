#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/core/events/EventStream.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <utility>

namespace TechEngine {
    EventStream::EventStream(const EventTypeId id, const std::uint32_t elementSize, const std::uint32_t alignment, const std::size_t initialCapacity) : m_id(id), m_elementSize(elementSize) {
        TE_CHECK(m_elementSize > 0);
        TE_CHECK(alignment <= alignof(std::max_align_t));

        m_visible = makeBuffer(initialCapacity);
        m_staging = makeBuffer(initialCapacity);
    }

    std::span<const std::byte> EventStream::readBytes() const {
        if (m_visible.count > 0) {
            return std::span<const std::byte>{m_visible.storage.get(), m_visible.count * m_elementSize};
        }
        return {};
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

    EventStream::Buffer EventStream::makeBuffer(const std::size_t capacity) const {
        return Buffer{std::make_unique<std::byte[]>(capacity * m_elementSize), capacity, 0};
    }

    void EventStream::stage(const void* event) {
        if (m_staging.count == m_staging.capacity) {
            grow();
        }

        std::memcpy(m_staging.storage.get() + m_staging.count * m_elementSize, event, m_elementSize);
        m_staging.count++;
    }

    void EventStream::grow() {
        Buffer grown = makeBuffer(std::max<std::size_t>(m_staging.capacity * 2, 1));

        std::memcpy(grown.storage.get(), m_staging.storage.get(), m_staging.count * m_elementSize);
        grown.count = m_staging.count;

        m_staging = std::move(grown);
    }
}
