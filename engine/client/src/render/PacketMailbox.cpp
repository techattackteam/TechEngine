#include <render/FramePacketPool.hpp>
#include <render/PacketHistory.hpp>
#include <render/PacketMailbox.hpp>

#include <utility>

namespace TechEngine {
    PacketMailbox::PacketMailbox(FramePacketPool& pool) : m_pool(&pool) {
    }

    PacketMailbox::~PacketMailbox() {
        reset();
    }

    void PacketMailbox::publish(std::unique_ptr<FramePacket> packet) {
        std::unique_ptr<FramePacket> displaced;
        {
            const std::lock_guard lock{m_mutex};
            displaced = std::exchange(m_packet, std::move(packet));
        }
        m_pool->release(std::move(displaced));
    }

    bool PacketMailbox::deliverTo(PacketHistory& history) {
        const std::lock_guard lock{m_mutex};
        if (m_packet) {
            history.acquire(std::move(m_packet));
            return true;
        }
        return false;
    }

    void PacketMailbox::reset() {
        std::unique_ptr<FramePacket> released;
        {
            const std::lock_guard lock{m_mutex};
            released = std::move(m_packet);
        }
        m_pool->release(std::move(released));
    }
}
