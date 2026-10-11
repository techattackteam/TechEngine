#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/resources/PackFormat.hpp>
#include <TechEngine/core/resources/PackReader.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    PackReader::PackReader(const FileAccess& files) : m_files(&files) {
    }

    std::error_code PackReader::open(const std::string_view virtualPath) {
        TE_PROFILER_FUNCTION();
        m_path.clear();
        m_entries.clear();
        m_bootRecord = PackEntry{};

        FileStatus fileStatus;
        const std::error_code statusError = m_files->status(virtualPath, fileStatus);
        if (statusError) {
            return statusError;
        }
        const std::uint64_t fileSize = fileStatus.size;

        std::vector<std::byte> headerBytes;
        const std::error_code headerError = m_files->readRange(virtualPath, 0, PACK_HEADER_SIZE, headerBytes);
        if (headerError == FileError::OutOfRange) {
            return ReadError::Truncated;
        }
        if (headerError) {
            return headerError;
        }

        Reader headerReader{headerBytes};
        BlobHeader header;
        std::uint32_t count = 0;
        std::uint64_t bootOffset = 0;
        std::uint64_t bootSize = 0;
        headerReader.readHeader(PACK_KIND, header);
        headerReader.read(count);
        headerReader.read(bootOffset);
        headerReader.read(bootSize);
        if (!headerReader.ok()) {
            return headerReader.error();
        }

        const std::uint64_t tableEnd = PACK_HEADER_SIZE + count * PACK_ENTRY_SIZE;
        std::vector<std::byte> tableBytes;
        const std::error_code tableError = m_files->readRange(virtualPath, PACK_HEADER_SIZE, count * PACK_ENTRY_SIZE, tableBytes);
        if (tableError == FileError::OutOfRange) {
            return ReadError::Truncated;
        }
        if (tableError) {
            return tableError;
        }

        Reader tableReader{tableBytes};
        std::vector<PackEntry> entries;
        entries.reserve(count);
        for (std::uint32_t index = 0; index < count; index++) {
            std::uint64_t id = 0;
            PackEntry entry;
            std::uint32_t nameOffset = 0;
            tableReader.read(id);
            tableReader.read(entry.typeTag);
            tableReader.read(entry.offset);
            tableReader.read(entry.size);
            tableReader.read(nameOffset);
            entry.id = ResourceId(id);

            if (!entry.id.valid() || (!entries.empty() && entry.id <= entries.back().id)) {
                return ReadError::Malformed;
            }
            if (entry.offset % PACK_ALIGNMENT != 0 || entry.offset < tableEnd || entry.offset > fileSize || entry.size > fileSize - entry.offset) {
                return ReadError::Malformed;
            }
            entries.push_back(entry);
        }
        if (!tableReader.ok()) {
            return tableReader.error();
        }

        if (bootSize != 0 && (bootOffset < tableEnd || bootOffset > fileSize || bootSize > fileSize - bootOffset)) {
            return ReadError::Malformed;
        }

        m_path = virtualPath;
        m_entries = std::move(entries);
        if (bootSize != 0) {
            m_bootRecord.offset = bootOffset;
            m_bootRecord.size = bootSize;
        }
        return {};
    }

    bool PackReader::find(const ResourceId id, PackEntry& out) const {
        const auto found = std::ranges::lower_bound(m_entries, id, {}, &PackEntry::id);
        if (found == m_entries.end() || found->id != id) {
            return false;
        }
        out = *found;
        return true;
    }

    bool PackReader::bootRecord(PackEntry& out) const {
        if (m_bootRecord.size == 0) {
            return false;
        }
        out = m_bootRecord;
        return true;
    }

    const std::vector<PackEntry>& PackReader::getEntries() const {
        return m_entries;
    }

    std::error_code PackReader::read(const PackEntry& entry, std::vector<std::byte>& out) const {
        TE_PROFILER_FUNCTION();
        return m_files->readRange(m_path, entry.offset, entry.size, out);
    }
}
