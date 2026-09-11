#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>

namespace TechEngine {
    InputBuffer::InputBuffer(const Clock& clock, const std::size_t capacity) : m_clock(clock), m_capacity(capacity) {
        TE_CHECK(capacity > 0, "Input capacity must be positive");
        m_pending.reserve(capacity);
    }

    void InputBuffer::publish(InputEvent event) {
        event.sequence = ++m_sequence;
        event.capturedAt = m_clock.now();
        {
            const std::lock_guard lock{m_presentationMutex};
            m_presentationState.apply(event);
        }

        const std::lock_guard lock{m_ingressMutex};
        m_mainState.apply(event);
        if (!m_overflowed && m_pending.size() == m_capacity) {
            m_overflowed = true;
            m_recoveryGeneration++;
            m_firstLostSequence = m_pending.front().sequence;
            m_pending.clear();
        }
        if (m_overflowed) {
            m_lastLostSequence = event.sequence;
        } else {
            m_pending.push_back(event);
        }
    }

    void InputBuffer::consume(InputFrame& frame) {
        frame.events.clear();
        if (frame.events.capacity() < m_capacity) {
            frame.events.reserve(m_capacity);
        }
        {
            const std::lock_guard lock{m_ingressMutex};
            frame.events.swap(m_pending);
            frame.recovered = m_overflowed;
            if (m_overflowed) {
                frame.recoveryGeneration = m_recoveryGeneration;
                frame.firstLostSequence = m_firstLostSequence;
                frame.lastLostSequence = m_lastLostSequence;
                frame.held = m_mainState;
                m_overflowed = false;
            }
        }
        if (!frame.recovered) {
            for (const InputEvent& event: frame.events) {
                frame.held.apply(event);
            }
        }
    }

    InputState InputBuffer::presentationState() const {
        const std::lock_guard lock{m_presentationMutex};
        return m_presentationState;
    }

    std::size_t InputBuffer::capacity() const {
        return m_capacity;
    }
}
