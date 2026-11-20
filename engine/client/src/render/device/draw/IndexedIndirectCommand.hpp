#pragma once

#include <cstdint>

namespace TechEngine {
    struct IndexedIndirectCommand {
        std::uint32_t indexCount = 0;
        std::uint32_t instanceCount = 0;
        std::uint32_t firstIndex = 0;
        std::int32_t baseVertex = 0;
        std::uint32_t baseInstance = 0;

        bool operator==(const IndexedIndirectCommand&) const = default;
    };

    static_assert(sizeof(IndexedIndirectCommand) == 20, "GL reads an indexed indirect command as five 32-bit words");
}
