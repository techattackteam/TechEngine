#include <TechEngine/base/diagnostics/Assert.hpp>

#include <render/FramePacketPool.hpp>

#include <utility>

namespace TechEngine {
    FramePacketPool::FramePacketPool() {
        for (std::size_t i = 0; i < PACKET_COUNT; i++) {
            m_free.push_back(std::make_unique<FramePacket>());
        }
    }

    std::unique_ptr<FramePacket> FramePacketPool::acquire() {
        const std::lock_guard lock{m_mutex};
        if (!m_free.empty()) {
            auto packet = std::move(m_free.back());
            m_free.pop_back();
            return packet;
        }
        TE_CHECK(false, "No free frame packets available");
        return nullptr;
    }

    void FramePacketPool::release(std::unique_ptr<FramePacket> packet) {
        const std::lock_guard lock{m_mutex};

        if (packet) {
            m_free.push_back(std::move(packet));
        }
    }

    std::size_t FramePacketPool::getAvailableCount() const {
        const std::lock_guard lock{m_mutex};
        return m_free.size();
    }
}
