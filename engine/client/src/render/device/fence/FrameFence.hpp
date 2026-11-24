#pragma once

#include <cstdint>

namespace TechEngine {
    struct GpuFrameFence {
        std::uint64_t frame = 0;
        void* sync = nullptr;
    };
}
