#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace TechEngine {
    class FileAccess;

    inline constexpr BlobKind PACK_KIND{0x4B504554, 1};

    inline constexpr std::uint64_t PACK_HEADER_SIZE = BLOB_HEADER_SIZE + 4 + 8 + 8;

    inline constexpr std::uint64_t PACK_ENTRY_SIZE = 8 + 8 + 8 + 8 + 4;

    inline constexpr std::uint64_t PACK_ALIGNMENT = 16;

    struct PackInput {
        ResourceId id;
        StringId typeTag;
        std::string_view name;
        std::span<const std::byte> payload;
    };

    struct PackEntry {
        ResourceId id;
        StringId typeTag;
        std::uint64_t offset = 0;
        std::uint64_t size = 0;
    };

    class PackWriter {
    private:
        std::vector<PackInput> m_entries;
        std::span<const std::byte> m_bootRecord;

    public:
        void add(const PackInput& input);

        void setBootRecord(std::span<const std::byte> bootRecord);

        void write(std::vector<std::byte>& out) const;
    };

    class PackReader {
    private:
        const FileAccess* m_files = nullptr;
        std::string m_path;
        std::vector<PackEntry> m_entries;
        PackEntry m_bootRecord;

    public:
        explicit PackReader(const FileAccess& files);

        std::error_code open(std::string_view virtualPath);

        bool find(ResourceId id, PackEntry& out) const;

        bool bootRecord(PackEntry& out) const;

        std::error_code read(const PackEntry& entry, std::vector<std::byte>& out) const;
    };
}
