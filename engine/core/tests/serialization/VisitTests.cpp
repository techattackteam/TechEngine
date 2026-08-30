#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/Visit.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using TechEngine::BlobHeader;
using TechEngine::Reader;
using TechEngine::ReadStatus;
using TechEngine::StringId;
using TechEngine::Visitable;
using TechEngine::Writer;

namespace {
    struct Bounds {
        float minX = 0.0f;
        float minY = 0.0f;
        float minZ = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
        float maxZ = 0.0f;

        bool operator==(const Bounds&) const = default;
    };

    struct Mesh {
        StringId id;
        std::string name;
        Bounds bounds;
        std::vector<std::uint32_t> indices;
        std::uint32_t materialSlot = 0;

        bool operator==(const Mesh&) const = default;
    };

    struct Undescribed {
        int value = 0;
    };

    template<typename Archive>
    void visit(Archive& archive, Bounds& value) {
        static_assert(sizeof(Bounds) == 24, "Bounds changed shape; check that visit still covers every field.");

        archive.field(value.minX);
        archive.field(value.minY);
        archive.field(value.minZ);
        archive.field(value.maxX);
        archive.field(value.maxY);
        archive.field(value.maxZ);
    }

    // No sizeof guard: a type holding std::string or std::vector has a different sizeof per
    // standard library and per config, so pinning a constant here goes red on another leg.
    template<typename Archive>
    void visit(Archive& archive, Mesh& value) {
        archive.field(value.id);
        archive.field(value.name);
        archive.field(value.bounds);
        archive.field(value.indices);
        archive.field(value.materialSlot);
    }

    Mesh makeMesh() {
        Mesh mesh;
        mesh.id = StringId{"TechEngine.DemoMesh"};
        mesh.name = "meshes/crate";
        mesh.bounds = Bounds{-1.0f, -2.0f, -3.0f, 1.5f, 2.5f, 3.5f};
        mesh.indices = {0, 1, 2, 2, 3, 0};
        mesh.materialSlot = 7;
        return mesh;
    }
}

TEST_CASE("the concept sees a described type and rejects an undescribed one", "[core][serialization]") {
    STATIC_CHECK(Visitable<Bounds, Writer>);
    STATIC_CHECK(Visitable<Bounds, Reader>);
    STATIC_CHECK(Visitable<Mesh, Writer>);
    STATIC_CHECK(Visitable<Mesh, Reader>);
    STATIC_CHECK_FALSE(Visitable<Undescribed, Writer>);
    STATIC_CHECK_FALSE(Visitable<Undescribed, Reader>);
}

TEST_CASE("a non-POD struct round-trips through both archives", "[core][serialization]") {
    const Mesh original = makeMesh();
    Mesh source = original;

    std::vector<std::byte> buffer;
    Writer writer{buffer};
    writer.field(source);

    Mesh restored;
    Reader reader{buffer};
    reader.field(restored);

    REQUIRE(reader.ok());
    CHECK(reader.remaining() == 0);
    CHECK(restored == original);
}

TEST_CASE("a visited struct round-trips behind the blob header", "[core][serialization]") {
    Mesh source = makeMesh();

    std::vector<std::byte> buffer;
    Writer writer{buffer};
    writer.writeHeader();
    writer.field(source);

    Reader reader{buffer};
    BlobHeader header;
    reader.readHeader(header);

    Mesh restored;
    reader.field(restored);

    REQUIRE(reader.ok());
    CHECK(header.magic == TechEngine::BLOB_MAGIC);
    CHECK(header.formatVersion == TechEngine::BLOB_FORMAT_VERSION);
    CHECK(restored == source);
}

TEST_CASE("a visit writes its fields in order, and that order is the format", "[core][serialization]") {
    Mesh source = makeMesh();

    std::vector<std::byte> viaVisit;
    Writer visiting{viaVisit};
    visiting.field(source);

    std::vector<std::byte> byHand;
    Writer manual{byHand};
    manual.write(source.id);
    manual.write(std::string_view{source.name});
    manual.write(source.bounds.minX);
    manual.write(source.bounds.minY);
    manual.write(source.bounds.minZ);
    manual.write(source.bounds.maxX);
    manual.write(source.bounds.maxY);
    manual.write(source.bounds.maxZ);
    manual.writeSpan(std::span<const std::uint32_t>{source.indices});
    manual.write(source.materialSlot);

    CHECK(viaVisit == byHand);
}

TEST_CASE("encoding a visited struct is deterministic", "[core][serialization]") {
    Mesh first = makeMesh();
    Mesh second = makeMesh();

    std::vector<std::byte> firstBuffer;
    Writer firstWriter{firstBuffer};
    firstWriter.field(first);

    std::vector<std::byte> secondBuffer;
    Writer secondWriter{secondBuffer};
    secondWriter.field(second);

    CHECK(firstBuffer == secondBuffer);
}

TEST_CASE("a truncated buffer fails the whole visit soft", "[core][serialization]") {
    Mesh source = makeMesh();

    std::vector<std::byte> buffer;
    Writer writer{buffer};
    writer.field(source);

    const std::span<const std::byte> truncated{buffer.data(), buffer.size() / 2};

    Mesh restored;
    Reader reader{truncated};
    reader.field(restored);

    CHECK_FALSE(reader.ok());
    CHECK(reader.status() == ReadStatus::Truncated);
}

TEST_CASE("the sticky status lets a visit body check once at the end", "[core][serialization]") {
    std::vector<std::byte> empty;

    Mesh restored;
    restored.materialSlot = 99;

    Reader reader{empty};
    reader.field(restored);

    CHECK(reader.status() == ReadStatus::Truncated);
    CHECK(restored.materialSlot == 99);
}
