#pragma once

#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/platform/input/InputState.hpp>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace TechEngine {
    struct InputFrame {
        std::vector<InputEvent> events;
        InputState held;
        bool recovered = false;
        std::uint64_t recoveryGeneration = 0;
        std::uint64_t firstLostSequence = 0;
        std::uint64_t lastLostSequence = 0;
    };

    class InputBuffer {
    private:
        const Clock& m_clock;
        std::size_t m_capacity;
        std::uint64_t m_sequence = 0;

        std::mutex m_ingressMutex;
        std::vector<InputEvent> m_pending;
        InputState m_mainState;
        bool m_overflowed = false;
        std::uint64_t m_recoveryGeneration = 0;
        std::uint64_t m_firstLostSequence = 0;
        std::uint64_t m_lastLostSequence = 0;

        mutable std::mutex m_presentationMutex;
        InputState m_presentationState;

    public:
        static constexpr std::size_t DEFAULT_CAPACITY = 256;

        explicit InputBuffer(const Clock& clock, std::size_t capacity = DEFAULT_CAPACITY);

        InputBuffer(const InputBuffer&) = delete;

        InputBuffer& operator=(const InputBuffer&) = delete;

        InputBuffer(InputBuffer&&) = delete;

        InputBuffer& operator=(InputBuffer&&) = delete;

        // One publishing thread only: the sequence counter is not synchronized.
        void publish(InputEvent event);

        void consume(InputFrame& frame);

        InputState presentationState() const;

        std::size_t capacity() const;
    };
}
