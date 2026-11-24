#pragma once

#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <cstddef>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

namespace TechEngineTests {
    inline std::vector<std::byte> assetBytes(const TechEngine::BlobKind& kind, const TechEngine::ResourceId id, const std::string_view payload) {
        std::vector<std::byte> bytes;
        TechEngine::Writer writer{bytes};
        writer.writeHeader(kind);
        writer.write(id.value());
        writer.writeBytes(std::as_bytes(std::span<const char>{payload.data(), payload.size()}));
        return bytes;
    }

    inline void writeBytes(const ScratchDirectory& scratch, const std::filesystem::path& relative, const std::vector<std::byte>& bytes) {
        scratch.writeFile(relative, std::string_view{reinterpret_cast<const char*>(bytes.data()), bytes.size()});
    }
}
