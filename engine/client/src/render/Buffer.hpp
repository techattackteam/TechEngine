#pragma once

#include <cstddef>
#include <span>

namespace TechEngine {
    class Buffer {
    private:
        unsigned int m_id = 0;

    public:
        Buffer() = default;

        ~Buffer();

        Buffer(const Buffer&) = delete;

        Buffer& operator=(const Buffer&) = delete;

        Buffer(Buffer&&) = delete;

        Buffer& operator=(Buffer&&) = delete;

        bool initialize(std::span<const std::byte> data);

        unsigned int id() const;

        void shutdown();
    };
}
