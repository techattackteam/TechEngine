#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/resources/PackFormat.hpp>
#include <TechEngine/core/resources/PackWriter.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace TechEngine {
    void PackWriter::add(const PackInput& input) {
        TE_CHECK(input.id != ResourceId{}, "Invalid resource ID");
        const bool duplicate = std::ranges::any_of(m_entries, [&input](const PackInput& entry) {
            return entry.id == input.id;
        });
        TE_CHECK(!duplicate, "Resource {0:#018x} is already in this pack", input.id.value());
        m_entries.push_back(input);
    }

    void PackWriter::setBootRecord(const std::span<const std::byte> bootRecord) {
        m_bootRecord = bootRecord;
    }

    void PackWriter::write(std::vector<std::byte>& out) const {
        TE_PROFILER_FUNCTION();
        TE_CHECK(m_entries.size() <= BLOB_MAX_COUNT, "A pack holds at most {0} entries, this one has {1}", BLOB_MAX_COUNT, m_entries.size());

        std::vector<PackInput> sortedEntries = m_entries;
        std::ranges::sort(sortedEntries, [](const PackInput& a, const PackInput& b) {
            return a.id < b.id;
        });

        const auto alignUp = [](const std::uint64_t offset) {
            return (offset + PACK_ALIGNMENT - 1) / PACK_ALIGNMENT * PACK_ALIGNMENT;
        };

        std::vector<std::uint32_t> nameOffsets;
        nameOffsets.reserve(sortedEntries.size());
        std::uint64_t namesSize = 0;
        for (const PackInput& entry: sortedEntries) {
            nameOffsets.push_back(static_cast<std::uint32_t>(namesSize));
            namesSize += sizeof(std::uint32_t) + entry.name.size();
        }
        TE_CHECK(namesSize <= BLOB_MAX_COUNT, "A pack's names block of {0} bytes exceeds the u32 name offset", namesSize);

        std::vector<std::uint64_t> payloadOffsets;
        payloadOffsets.reserve(sortedEntries.size());
        std::uint64_t position = PACK_HEADER_SIZE + sortedEntries.size() * PACK_ENTRY_SIZE + namesSize;
        for (const PackInput& entry: sortedEntries) {
            position = alignUp(position);
            payloadOffsets.push_back(position);
            position += entry.payload.size();
        }
        std::uint64_t bootOffset = 0;
        if (!m_bootRecord.empty()) {
            bootOffset = alignUp(position);
        }

        const std::size_t start = out.size();
        Writer writer{out};
        writer.writeHeader(PACK_KIND);
        writer.write(static_cast<std::uint32_t>(sortedEntries.size()));
        writer.write(bootOffset);
        writer.write(static_cast<std::uint64_t>(m_bootRecord.size()));

        for (std::size_t index = 0; index < sortedEntries.size(); index++) {
            const PackInput& entry = sortedEntries[index];
            writer.write(entry.id.value());
            writer.write(entry.typeTag);
            writer.write(payloadOffsets[index]);
            writer.write(static_cast<std::uint64_t>(entry.payload.size()));
            writer.write(nameOffsets[index]);
        }

        for (const PackInput& entry: sortedEntries) {
            writer.write(entry.name);
        }

        for (std::size_t index = 0; index < sortedEntries.size(); index++) {
            out.resize(start + static_cast<std::size_t>(payloadOffsets[index]));
            writer.writeBytes(sortedEntries[index].payload);
        }

        if (!m_bootRecord.empty()) {
            out.resize(start + static_cast<std::size_t>(bootOffset));
            writer.writeBytes(m_bootRecord);
        }
    }
}
