#include <render/FrameCommandBuffer.hpp>

namespace TechEngine {
    void FrameCommandBuffer::publish(const FrameCommand& command) {
        m_mutex.lock();
        m_command = command;
        m_mutex.unlock();
    }

    FrameCommand FrameCommandBuffer::snapshot() const {
        m_mutex.lock();
        FrameCommand result = m_command;
        m_mutex.unlock();
        return result;
    }

    void FrameCommandBuffer::reset() {
        m_mutex.lock();
        m_command = FrameCommand{};
        m_mutex.unlock();
    }
}
