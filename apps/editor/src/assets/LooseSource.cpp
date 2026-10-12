#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/LooseSource.hpp>

namespace TechEngine {
    LooseSource::LooseSource(const FileAccess& files, const AssetIndex& index) : m_files(&files), m_index(&index) {
    }

    AssetAvailability LooseSource::find(const ResourceId id) const {
        AssetRecord record;
        if (!m_index->find(id, record)) {
            return AssetAvailability::Unknown;
        }
        if (record.side == Side::Server) {
            return AssetAvailability::AbsentForRole;
        }
        return AssetAvailability::Present;
    }

    std::error_code LooseSource::read(const ResourceId id, std::vector<std::byte>& out) const {
        AssetRecord record;
        const bool indexed = m_index->find(id, record);
        TE_CHECK(indexed && record.side != Side::Server, "LooseSource: ID {0:#018x} is not Present", id.value());

        return m_files->read(record.path, out);
    }
}
