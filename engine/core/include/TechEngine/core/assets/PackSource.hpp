#pragma once

#include <TechEngine/core/assets/AssetSource.hpp>
#include <TechEngine/core/resources/PackReader.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <system_error>
#include <vector>

namespace TechEngine {
    class FileAccess;
    struct BootRecord;
    enum class Role : std::uint8_t;

    class PackSource final : public AssetSource {
    private:
        struct Location {
            ResourceId id;
            std::uint32_t pack = 0;
            std::uint64_t offset = 0;
            std::uint64_t size = 0;
        };

        const FileAccess* m_files = nullptr;
        std::vector<PackReader> m_packs;
        std::vector<Location> m_table;
        std::vector<ResourceId> m_known;

    public:
        explicit PackSource(const FileAccess& files);

        std::error_code open(const BootRecord& record, Role role, std::string_view directory);

        AssetAvailability find(ResourceId id) const override;

        std::error_code read(ResourceId id, std::vector<std::byte>& out) const override;
    };
}
