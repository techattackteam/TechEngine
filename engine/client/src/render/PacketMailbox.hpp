#pragma once

#include <TechEngine/client/render/FramePacket.hpp>

#include <memory>
#include <mutex>

namespace TechEngine {
    class FramePacketPool;
    class PacketHistory;

    class PacketMailbox {
    private:
        FramePacketPool* m_pool;
        std::mutex m_mutex;
        std::unique_ptr<FramePacket> m_packet;

    public:
        // The pool must outlive the mailbox, which returns its packet there when it dies.
        explicit PacketMailbox(FramePacketPool& pool);

        ~PacketMailbox();

        PacketMailbox(const PacketMailbox&) = delete;

        PacketMailbox& operator=(const PacketMailbox&) = delete;

        PacketMailbox(PacketMailbox&&) = delete;

        PacketMailbox& operator=(PacketMailbox&&) = delete;

        void publish(std::unique_ptr<FramePacket> packet);

        // The history acquires while the lock is held. Taking the packet out first would let
        // simulation publish and acquire while render holds three packets, which empties the pool.
        bool deliverTo(PacketHistory& history);

        void reset();
    };
}
