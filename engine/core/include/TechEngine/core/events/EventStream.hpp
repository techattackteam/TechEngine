#pragma once

#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/core/events/EventTypeId.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace TechEngine {
    struct EventCursor {
        std::uint64_t sequence{0};
    };

    struct EventBatchMark {
        std::uint64_t endSequence{0};
        std::uint64_t frameIndex{0};
        std::uint64_t tick{0};
    };

    class EventStream {
    private:
        struct ByteRange {
            const std::byte* data{nullptr};
            std::size_t count{0};
        };

        EventTypeId m_id;
        std::uint32_t m_elementSize = 0;
        std::uint32_t m_alignment{0};
        std::size_t m_capacity{0};
        std::unique_ptr<std::byte[]> m_storage;
        std::vector<EventBatchMark> m_marks;
        std::uint64_t m_retireHeadSequence{0};
        std::uint64_t m_visibleEndSequence{0};
        std::uint64_t m_stagingTailSequence{0};

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
        std::span<const T> read(EventCursor& cursor) const {
            TE_ASSERT(m_id == eventTypeId<T>());
            const ByteRange range = visibleFrom(cursor);
            return std::span<const T>{reinterpret_cast<const T*>(range.data), range.count};
        }

        void makeVisible(std::uint64_t frameIndex, std::uint64_t tick);

        void retire(std::uint64_t frameIndex, std::uint64_t tick);

        EventTypeId id() const;

        std::size_t visibleCount() const;

        std::size_t stagedCount() const;

        std::size_t capacity() const;

        std::uint64_t retireHeadSequence() const;

        std::uint64_t visibleEndSequence() const;

    private:
        void stage(const void* event);

        ByteRange visibleFrom(EventCursor& cursor) const;

        void grow(std::size_t minimumCapacity);
    };
}
