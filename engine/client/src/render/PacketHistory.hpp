#pragma once

#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/client/render/FramePacket.hpp>

#include <render/FrameView.hpp>

#include <memory>

namespace TechEngine {
    class FramePacketPool;

    class PacketHistory {
    private:
        FramePacketPool* m_pool;
        std::unique_ptr<FramePacket> m_previous;
        std::unique_ptr<FramePacket> m_current;

    public:
        // The pool must outlive the history, which returns its packets there when it dies.
        explicit PacketHistory(FramePacketPool& pool);

        ~PacketHistory();

        PacketHistory(const PacketHistory&) = delete;

        PacketHistory& operator=(const PacketHistory&) = delete;

        PacketHistory(PacketHistory&&) = delete;

        PacketHistory& operator=(PacketHistory&&) = delete;

        void acquire(std::unique_ptr<FramePacket> packet);

        double alpha(Clock::TimePoint now) const;

        void prepareFrame(Clock::TimePoint now, const InputState& input, FrameView& out) const;

    private:
        void interpolateScene(float blend, FrameView& out) const;
    };
}
