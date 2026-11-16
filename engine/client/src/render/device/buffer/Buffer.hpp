#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace TechEngine {
    enum class BufferUsage : std::uint8_t { Static, Dynamic };

    struct BufferDescription {
        std::size_t size = 0;
        BufferUsage usage = BufferUsage::Static;
        std::span<const std::byte> initialData;
    };

    struct GpuBuffer {
        unsigned int id = 0;
        std::size_t size = 0;
        BufferUsage usage = BufferUsage::Static;
    };
}
