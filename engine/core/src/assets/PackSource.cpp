#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/assets/PackSource.hpp>
#include <TechEngine/core/resources/BootRecord.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    PackSource::PackSource(const FileAccess& files) : m_files(&files) {
    }

    std::error_code PackSource::open(const BootRecord& record, const Role role, const std::string_view directory) {
        TE_PROFILER_FUNCTION();
        m_packs.clear();
        m_table.clear();
        m_known.clear();

        const auto rolePacks = std::ranges::find(record.rolePacks, role, &RolePacks::role);
        if (rolePacks == record.rolePacks.end()) {
            return ReadError::Malformed;
        }

        std::vector<PackReader> packs;
        std::vector<Location> table;
        packs.reserve(rolePacks->packs.size());
        for (const std::string& name: rolePacks->packs) {
            PackReader reader{*m_files};
            const std::error_code openError = reader.open(std::string(directory) + name);
            if (openError) {
                return openError;
            }

            const auto packIndex = static_cast<std::uint32_t>(packs.size());
            for (const PackEntry& entry: reader.getEntries()) {
                table.push_back({entry.id, packIndex, entry.offset, entry.size});
            }
            packs.push_back(std::move(reader));
        }

        std::ranges::sort(table, {}, &Location::id);
        const auto duplicate = std::ranges::adjacent_find(table, std::ranges::equal_to{}, &Location::id);
        if (duplicate != table.end()) {
            return ReadError::Malformed;
        }

        std::vector<ResourceId> known;
        known.reserve(record.sides.size());
        for (const ResourceSide& side: record.sides) {
            known.push_back(side.id);
        }
        std::ranges::sort(known);

        m_packs = std::move(packs);
        m_table = std::move(table);
        m_known = std::move(known);
        return {};
    }

    AssetAvailability PackSource::find(const ResourceId id) const {
        if (std::ranges::binary_search(m_table, id, {}, &Location::id)) {
            return AssetAvailability::Present;
        }
        if (std::ranges::binary_search(m_known, id)) {
            return AssetAvailability::AbsentForRole;
        }
        return AssetAvailability::Unknown;
    }

    std::error_code PackSource::read(const ResourceId id, std::vector<std::byte>& out) const {
        TE_PROFILER_FUNCTION();
        const auto found = std::ranges::lower_bound(m_table, id, {}, &Location::id);
        TE_CHECK(found != m_table.end() && found->id == id, "PackSource: ID {0:#018x} is not Present", id.value());

        const PackEntry entry{found->id, StringId{}, found->offset, found->size};
        return m_packs[found->pack].read(entry, out);
    }
}
