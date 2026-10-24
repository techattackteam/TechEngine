#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

namespace TechEngine {
    static_assert(sizeof(Vec3) == 3 * sizeof(float) && std::is_trivially_copyable_v<Vec3>, "A mesh writes its positions as packed float3 values");

    void Mesh::write(std::vector<std::byte>& out) const {
        TE_PROFILER_FUNCTION();
        Writer writer{out};
        writer.writeHeader(MESH_KIND);
        writer.write(id.value());
        writer.writeSpan(std::span<const Vec3>{positions});
        writer.writeSpan(std::span<const std::uint32_t>{indices});
    }

    std::error_code Mesh::read(const std::span<const std::byte> bytes) {
        TE_PROFILER_FUNCTION();
        Reader reader{bytes};
        BlobHeader header;
        reader.readHeader(MESH_KIND, header);
        std::uint64_t idValue = 0;
        reader.read(idValue);
        std::vector<Vec3> readPositions;
        reader.readSpan(readPositions);
        std::vector<std::uint32_t> readIndices;
        reader.readSpan(readIndices);
        if (!reader.ok()) {
            return reader.error();
        }
        if (reader.remaining() != 0 || readIndices.size() % 3 != 0) {
            return ReadError::Malformed;
        }
        const bool inRange = std::ranges::all_of(readIndices, [&readPositions](const std::uint32_t index) {
            return index < readPositions.size();
        });
        if (!inRange) {
            return ReadError::Malformed;
        }

        id = ResourceId{idValue};
        positions = std::move(readPositions);
        indices = std::move(readIndices);
        return {};
    }

    std::error_code loadMesh(const std::span<const std::byte> bytes, void* payload, std::vector<ResourceId>& dependencies) {
        Mesh* mesh = static_cast<Mesh*>(payload);
        dependencies.clear();
        return mesh->read(bytes);
    }
}
