#pragma once

#include <TechEngine/client/render/FrameCommand.hpp>

#include <mutex>

namespace TechEngine {
    class FrameCommandBuffer {
    private:
        mutable std::mutex m_mutex;
        FrameCommand m_command;

    public:
        void publish(const FrameCommand& command);

        FrameCommand snapshot() const;

        void reset();
    };
}
