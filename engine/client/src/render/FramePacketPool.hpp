#pragma once

#include <TechEngine/client/render/FramePacket.hpp>

#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace TechEngine {
    class FramePacketPool {
    public:
        static constexpr std::size_t PACKET_COUNT = 4;

    private:
        mutable std::mutex m_mutex;
        std::vector<std::unique_ptr<FramePacket>> m_free;

    public:
        FramePacketPool();

        FramePacketPool(const FramePacketPool&) = delete;

        FramePacketPool& operator=(const FramePacketPool&) = delete;

        FramePacketPool(FramePacketPool&&) = delete;

        FramePacketPool& operator=(FramePacketPool&&) = delete;

        // An empty pool is misuse: it fires a check and returns null.
        std::unique_ptr<FramePacket> acquire();

        void release(std::unique_ptr<FramePacket> packet);

        std::size_t getAvailableCount() const;
    };
}
