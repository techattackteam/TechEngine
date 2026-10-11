#pragma once

#include <TechEngine/core/serialization/BlobHeader.hpp>

#include <cstdint>

namespace TechEngine {
    inline constexpr BlobKind PACK_KIND{0x4B504554, 1};

    inline constexpr std::uint64_t PACK_HEADER_SIZE = BLOB_HEADER_SIZE + 4 + 8 + 8;

    inline constexpr std::uint64_t PACK_ENTRY_SIZE = 8 + 8 + 8 + 8 + 4;

    inline constexpr std::uint64_t PACK_ALIGNMENT = 16;
}
