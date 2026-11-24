#pragma once

#include <TechEngine/core/assets/AssetSource.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>

#include <cstddef>
#include <system_error>
#include <vector>

namespace TechEngine {
    class AssetIndex;
    class FileAccess;

    class LooseSource final : public AssetSource {
    private:
        const FileAccess* m_files = nullptr;
        const AssetIndex* m_index = nullptr;

    public:
        LooseSource(const FileAccess& files, const AssetIndex& index);

        AssetAvailability find(ResourceId id) const override;

        std::error_code read(ResourceId id, std::vector<std::byte>& out) const override;
    };
}
