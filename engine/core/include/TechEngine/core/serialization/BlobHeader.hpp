#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>

namespace TechEngine {
    static_assert(std::endian::native == std::endian::little, "TechEngine blobs are little-endian on disk and wire; no swap path exists.");

    inline constexpr std::uint32_t BLOB_MAGIC = 'T' | ('E' << 8) | ('C' << 16) | ('H' << 24);

    inline constexpr std::uint16_t BLOB_FORMAT_VERSION = 1;

    inline constexpr std::uint32_t BLOB_HEADER_SIZE = 8;

    inline constexpr std::size_t BLOB_MAX_COUNT = 0xFFFFFFFF;

    struct BlobHeader {
        std::uint32_t magic = 0;
        std::uint16_t formatVersion = 0;
        std::uint16_t flags = 0;
    };
}
