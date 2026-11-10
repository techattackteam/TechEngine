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
    static_assert(sizeof(Vec2) == 2 * sizeof(float) && std::is_trivially_copyable_v<Vec2>, "A mesh writes its UVs as packed float2 values");
    static_assert(sizeof(Vec3) == 3 * sizeof(float) && std::is_trivially_copyable_v<Vec3>, "A mesh writes its positions and normals as packed float3 values");
    static_assert(sizeof(Vec4) == 4 * sizeof(float) && std::is_trivially_copyable_v<Vec4>, "A mesh writes its tangents as packed float4 values");

    static constexpr std::uint32_t MESH_STREAM_NORMAL = 1u << 0;
    static constexpr std::uint32_t MESH_STREAM_TANGENT = 1u << 1;
    static constexpr std::uint32_t MESH_STREAM_UV0 = 1u << 2;
    static constexpr std::uint32_t MESH_KNOWN_STREAMS = MESH_STREAM_NORMAL | MESH_STREAM_TANGENT | MESH_STREAM_UV0;
    static constexpr std::uint64_t MESH_SLOT_MIN_SIZE = sizeof(std::uint32_t) + sizeof(std::uint32_t) + sizeof(std::uint32_t) + sizeof(std::uint64_t);

    void Mesh::write(std::vector<std::byte>& out) const {
        TE_PROFILER_FUNCTION();
        std::uint32_t streams = 0;
        if (!normals.empty()) {
            streams |= MESH_STREAM_NORMAL;
        }
        if (!tangents.empty()) {
            streams |= MESH_STREAM_TANGENT;
        }
        if (!uv0.empty()) {
            streams |= MESH_STREAM_UV0;
        }

        Writer writer{out};
        writer.writeHeader(MESH_KIND);
        writer.write(id.value());
        importBlock.write(writer);
        writer.write(streams);
        writer.writeSpan(std::span<const Vec3>{positions});
        if ((streams & MESH_STREAM_NORMAL) != 0) {
            writer.writeSpan(std::span<const Vec3>{normals});
        }
        if ((streams & MESH_STREAM_TANGENT) != 0) {
            writer.writeSpan(std::span<const Vec4>{tangents});
        }
        if ((streams & MESH_STREAM_UV0) != 0) {
            writer.writeSpan(std::span<const Vec2>{uv0});
        }
        writer.writeSpan(std::span<const std::uint32_t>{indices});

        writer.write(static_cast<std::uint32_t>(slots.size()));
        for (const MeshSlot& slot: slots) {
            writer.write(slot.name);
            writer.write(slot.firstIndex);
            writer.write(slot.indexCount);
            writer.write(slot.defaultMaterial.value());
        }
    }

    std::error_code Mesh::read(const std::span<const std::byte> bytes) {
        TE_PROFILER_FUNCTION();
        Reader reader{bytes};
        BlobHeader header;
        reader.readHeader(MESH_KIND, header);
        std::uint64_t idValue = 0;
        reader.read(idValue);
        if (!reader.ok()) {
            return reader.error();
        }

        ImportBlock readImportBlock;
        const std::error_code importError = readImportBlock.read(reader);
        if (importError) {
            return importError;
        }

        std::uint32_t streams = 0;
        reader.read(streams);
        if (!reader.ok()) {
            return reader.error();
        }
        if ((streams & ~MESH_KNOWN_STREAMS) != 0) {
            return ReadError::Malformed;
        }

        std::vector<Vec3> readPositions;
        reader.readSpan(readPositions);
        std::vector<Vec3> readNormals;
        if ((streams & MESH_STREAM_NORMAL) != 0) {
            reader.readSpan(readNormals);
        }
        std::vector<Vec4> readTangents;
        if ((streams & MESH_STREAM_TANGENT) != 0) {
            reader.readSpan(readTangents);
        }
        std::vector<Vec2> readUv0;
        if ((streams & MESH_STREAM_UV0) != 0) {
            reader.readSpan(readUv0);
        }
        std::vector<std::uint32_t> readIndices;
        reader.readSpan(readIndices);

        std::uint32_t slotCount = 0;
        reader.read(slotCount);
        if (!reader.ok()) {
            return reader.error();
        }
        if (static_cast<std::uint64_t>(slotCount) * MESH_SLOT_MIN_SIZE > reader.remaining()) {
            return ReadError::Truncated;
        }

        std::vector<MeshSlot> readSlots(slotCount);
        for (MeshSlot& slot: readSlots) {
            reader.read(slot.name);
            reader.read(slot.firstIndex);
            reader.read(slot.indexCount);
            std::uint64_t materialValue = 0;
            reader.read(materialValue);
            slot.defaultMaterial = ResourceId{materialValue};
        }
        if (!reader.ok()) {
            return reader.error();
        }
        if (reader.remaining() != 0) {
            return ReadError::Malformed;
        }

        // A set flag with a count of zero is a stream that does not cover its vertices, not an absent one.
        const std::size_t vertexCount = readPositions.size();
        const bool normalsMatch = (streams & MESH_STREAM_NORMAL) == 0 || readNormals.size() == vertexCount;
        const bool tangentsMatch = (streams & MESH_STREAM_TANGENT) == 0 || readTangents.size() == vertexCount;
        const bool uv0Matches = (streams & MESH_STREAM_UV0) == 0 || readUv0.size() == vertexCount;
        if (!normalsMatch || !tangentsMatch || !uv0Matches || readIndices.size() % 3 != 0) {
            return ReadError::Malformed;
        }
        const bool inRange = std::ranges::all_of(readIndices, [vertexCount](const std::uint32_t index) {
            return index < vertexCount;
        });
        if (!inRange) {
            return ReadError::Malformed;
        }

        if (readSlots.empty()) {
            return ReadError::Malformed;
        }
        std::uint64_t covered = 0;
        for (const MeshSlot& slot: readSlots) {
            if (slot.firstIndex != covered || slot.indexCount % 3 != 0) {
                return ReadError::Malformed;
            }
            covered += slot.indexCount;
        }
        if (covered != readIndices.size()) {
            return ReadError::Malformed;
        }

        id = ResourceId{idValue};
        importBlock = std::move(readImportBlock);
        positions = std::move(readPositions);
        normals = std::move(readNormals);
        tangents = std::move(readTangents);
        uv0 = std::move(readUv0);
        indices = std::move(readIndices);
        slots = std::move(readSlots);
        return {};
    }

    std::error_code Mesh::loadPayload(const std::span<const std::byte> bytes, void* payload, std::vector<ResourceId>& dependencies) {
        Mesh* mesh = static_cast<Mesh*>(payload);
        dependencies.clear();
        const std::error_code readError = mesh->read(bytes);
        if (readError) {
            return readError;
        }

        for (const MeshSlot& slot: mesh->slots) {
            if (slot.defaultMaterial.valid()) {
                dependencies.push_back(slot.defaultMaterial);
            }
        }
        return {};
    }
}
