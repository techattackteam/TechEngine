#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/core/events/EventStream.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <utility>

namespace TechEngine {
    static constexpr std::size_t INITIAL_MARK_CAPACITY = 64;

    EventStream::EventStream(const EventTypeId id, const std::uint32_t elementSize, const std::uint32_t alignment, const std::size_t initialCapacity) : m_id{id}, m_elementSize{elementSize}, m_alignment{alignment}, m_capacity{initialCapacity} {
        TE_CHECK(m_elementSize > 0);
        TE_CHECK(m_alignment <= alignof(std::max_align_t));

        m_storage = std::make_unique<std::byte[]>(m_capacity * m_elementSize);
        m_marks.reserve(INITIAL_MARK_CAPACITY);
    }

    void EventStream::makeVisible(const std::uint64_t frameIndex, const std::uint64_t tick) {
        if (m_stagingTailSequence == m_visibleEndSequence) {
            return;
        }

        m_visibleEndSequence = m_stagingTailSequence;
        m_marks.push_back(EventBatchMark{m_visibleEndSequence, frameIndex, tick});
    }

    void EventStream::retire(const std::uint64_t frameIndex, const std::uint64_t tick) {
        std::size_t retiredMarks = 0;
        std::uint64_t newHead = m_retireHeadSequence;

        for (const EventBatchMark& mark: m_marks) {
            if (frameIndex <= mark.frameIndex || tick <= mark.tick) {
                break;
            }
            newHead = mark.endSequence;
            retiredMarks++;
        }

        if (retiredMarks == 0) {
            return;
        }

        m_marks.erase(m_marks.begin(), m_marks.begin() + static_cast<std::ptrdiff_t>(retiredMarks));

        const std::size_t retained = m_stagingTailSequence - newHead;
        const std::size_t offset = newHead - m_retireHeadSequence;
        std::memmove(m_storage.get(), m_storage.get() + offset * m_elementSize, retained * m_elementSize);

        m_retireHeadSequence = newHead;
    }

    EventTypeId EventStream::id() const {
        return m_id;
    }

    std::size_t EventStream::visibleCount() const {
        return m_visibleEndSequence - m_retireHeadSequence;
    }

    std::size_t EventStream::stagedCount() const {
        return m_stagingTailSequence - m_visibleEndSequence;
    }

    std::size_t EventStream::capacity() const {
        return m_capacity;
    }

    std::uint64_t EventStream::retireHeadSequence() const {
        return m_retireHeadSequence;
    }

    std::uint64_t EventStream::visibleEndSequence() const {
        return m_visibleEndSequence;
    }

    void EventStream::stage(const void* event) {
        const std::size_t used = m_stagingTailSequence - m_retireHeadSequence;
        if (used == m_capacity) {
            grow(m_capacity * 2);
        }

        std::memcpy(m_storage.get() + used * m_elementSize, event, m_elementSize);
        m_stagingTailSequence++;
    }

    EventStream::ByteRange EventStream::visibleFrom(EventCursor& cursor) const {
        if (cursor.sequence < m_retireHeadSequence) {
            cursor.sequence = m_retireHeadSequence;
        }
        if (cursor.sequence > m_visibleEndSequence) {
            cursor.sequence = m_visibleEndSequence;
        }

        const std::size_t offset = cursor.sequence - m_retireHeadSequence;
        const std::size_t count = m_visibleEndSequence - cursor.sequence;
        cursor.sequence = m_visibleEndSequence;

        return ByteRange{m_storage.get() + offset * m_elementSize, count};
    }

    void EventStream::grow(std::size_t minimumCapacity) {
        const std::size_t newCapacity = std::max<std::size_t>(minimumCapacity, 1);
        std::unique_ptr<std::byte[]> storage = std::make_unique<std::byte[]>(newCapacity * m_elementSize);

        const std::size_t used = m_stagingTailSequence - m_retireHeadSequence;
        std::memcpy(storage.get(), m_storage.get(), used * m_elementSize);

        m_storage = std::move(storage);
        m_capacity = newCapacity;
    }
}
