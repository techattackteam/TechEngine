#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/resources/BootRecord.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    void BootRecord::write(std::vector<std::byte>& out) const {
        TE_PROFILER_FUNCTION();
        Writer writer{out};
        writer.write(static_cast<std::uint32_t>(4));

        std::vector<std::byte> payload;
        Writer payloadWriter{payload};
        const auto writeSection = [&writer, &payload](const StringId tag) {
            TE_CHECK(payload.size() <= BLOB_MAX_COUNT, "Boot record section {0} of {1} bytes exceeds the u32 section length", tag, payload.size());
            writer.write(tag);
            writer.write(BOOT_SECTION_VERSION);
            writer.write(static_cast<std::uint32_t>(payload.size()));
            writer.writeBytes(payload);
            payload.clear();
        };

        payloadWriter.write(startupScene.value());
        writeSection(BOOT_STARTUP_SCENE_SECTION);

        payloadWriter.write(static_cast<std::uint32_t>(systemSettings.size()));
        for (const SystemSettingsEntry& entry: systemSettings) {
            payloadWriter.write(entry.system);
            payloadWriter.write(entry.settings.value());
        }
        writeSection(BOOT_SYSTEM_SETTINGS_SECTION);

        payloadWriter.write(static_cast<std::uint32_t>(rolePacks.size()));
        for (const RolePacks& entry: rolePacks) {
            payloadWriter.write(static_cast<std::uint8_t>(entry.role));
            payloadWriter.write(static_cast<std::uint32_t>(entry.packs.size()));
            for (const std::string& pack: entry.packs) {
                payloadWriter.write(pack);
            }
        }
        writeSection(BOOT_ROLE_PACKS_SECTION);

        payloadWriter.write(static_cast<std::uint32_t>(sides.size()));
        for (const ResourceSide& entry: sides) {
            TE_CHECK(entry.id.valid(), "A boot record's sides hold no null id");
            payloadWriter.write(entry.id.value());
            payloadWriter.write(static_cast<std::uint8_t>(entry.side));
        }
        writeSection(BOOT_SIDES_SECTION);
    }

    std::error_code BootRecord::read(const std::span<const std::byte> bytes) {
        TE_PROFILER_FUNCTION();
        Reader reader{bytes};
        std::uint32_t sectionCount = 0;
        reader.read(sectionCount);
        if (!reader.ok()) {
            return reader.error();
        }

        BootRecord record;
        std::vector<std::byte> payload;
        for (std::uint32_t index = 0; index < sectionCount; index++) {
            StringId tag;
            std::uint16_t version = 0;
            std::uint32_t length = 0;
            reader.read(tag);
            reader.read(version);
            reader.read(length);
            if (!reader.ok()) {
                return reader.error();
            }
            if (reader.remaining() < length) {
                return ReadError::Truncated;
            }
            payload.resize(length);
            reader.readBytes(payload);

            const bool known = tag == BOOT_STARTUP_SCENE_SECTION || tag == BOOT_SYSTEM_SETTINGS_SECTION || tag == BOOT_ROLE_PACKS_SECTION || tag == BOOT_SIDES_SECTION;
            if (!known) {
                TE_LOGGER_WARN("Boot record: skipped the unknown section {0}", tag);
                continue;
            }
            if (version != BOOT_SECTION_VERSION) {
                return ReadError::BadVersion;
            }

            Reader payloadReader{payload};
            const std::error_code sectionError = record.readSection(tag, payloadReader);
            if (sectionError) {
                return sectionError;
            }
            if (!payloadReader.ok() || payloadReader.remaining() != 0) {
                return ReadError::Malformed;
            }
        }

        *this = std::move(record);
        return {};
    }

    std::error_code BootRecord::readSection(const StringId tag, Reader& reader) {
        if (tag == BOOT_STARTUP_SCENE_SECTION) {
            std::uint64_t scene = 0;
            reader.read(scene);
            startupScene = ResourceId(scene);
            return {};
        }

        if (tag == BOOT_SYSTEM_SETTINGS_SECTION) {
            std::uint32_t count = 0;
            reader.read(count);
            for (std::uint32_t index = 0; index < count && reader.ok(); index++) {
                SystemSettingsEntry entry;
                std::uint64_t settings = 0;
                reader.read(entry.system);
                reader.read(settings);
                entry.settings = ResourceId(settings);
                systemSettings.push_back(std::move(entry));
            }
            return {};
        }

        if (tag == BOOT_ROLE_PACKS_SECTION) {
            std::uint32_t count = 0;
            reader.read(count);
            for (std::uint32_t index = 0; index < count && reader.ok(); index++) {
                std::uint8_t role = 0;
                std::uint32_t packCount = 0;
                reader.read(role);
                reader.read(packCount);
                if (reader.ok() && role > static_cast<std::uint8_t>(Role::DedicatedServer)) {
                    return ReadError::Malformed;
                }

                RolePacks entry;
                entry.role = static_cast<Role>(role);
                for (std::uint32_t pack = 0; pack < packCount && reader.ok(); pack++) {
                    std::string name;
                    reader.read(name);
                    entry.packs.push_back(std::move(name));
                }
                rolePacks.push_back(std::move(entry));
            }
            return {};
        }

        std::uint32_t count = 0;
        reader.read(count);
        for (std::uint32_t index = 0; index < count && reader.ok(); index++) {
            std::uint64_t id = 0;
            std::uint8_t side = 0;
            reader.read(id);
            reader.read(side);
            if (reader.ok() && (id == 0 || side > static_cast<std::uint8_t>(Side::Server))) {
                return ReadError::Malformed;
            }
            sides.push_back({ResourceId(id), static_cast<Side>(side)});
        }
        return {};
    }
}
