#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

using TechEngine::BlobHeader;
using TechEngine::Reader;
using TechEngine::ReadStatus;
using TechEngine::StringId;
using TechEngine::Writer;

namespace {
    struct Pixel {
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;
        std::uint8_t a = 0;

        bool operator==(const Pixel&) const = default;
    };
}

TEST_CASE("the header round-trips through the pair", "[core][serialization]") {
    std::vector<std::byte> buffer;
    Writer writer{buffer};
    writer.writeHeader();

    REQUIRE(buffer.size() == TechEngine::BLOB_HEADER_SIZE);

    Reader reader{buffer};
    BlobHeader header;
    reader.readHeader(header);

    REQUIRE(reader.ok());
    CHECK(header.magic == TechEngine::BLOB_MAGIC);
    CHECK(header.formatVersion == TechEngine::BLOB_FORMAT_VERSION);
    CHECK(header.flags == 0);
}

TEST_CASE("every primitive round-trips", "[core][serialization]") {
    std::vector<std::byte> buffer;
    Writer writer{buffer};

    writer.write(true);
    writer.write(static_cast<std::int8_t>(-8));
    writer.write(static_cast<std::uint8_t>(200));
    writer.write(static_cast<std::int16_t>(-300));
    writer.write(static_cast<std::uint16_t>(60000));
    writer.write(static_cast<std::int32_t>(-70000));
    writer.write(static_cast<std::uint32_t>(4000000000U));
    writer.write(static_cast<std::int64_t>(-5000000000LL));
    writer.write(static_cast<std::uint64_t>(18000000000000000000ULL));
    writer.write(0.5f);
    writer.write(0.25);
    writer.write(std::string_view{"TechEngine"});
    writer.write(StringId{"TechEngine.Transform"});

    Reader reader{buffer};

    bool flag = false;
    std::int8_t i8 = 0;
    std::uint8_t u8 = 0;
    std::int16_t i16 = 0;
    std::uint16_t u16 = 0;
    std::int32_t i32 = 0;
    std::uint32_t u32 = 0;
    std::int64_t i64 = 0;
    std::uint64_t u64 = 0;
    float f32 = 0.0f;
    double f64 = 0.0;
    std::string text;
    StringId id;

    reader.read(flag);
    reader.read(i8);
    reader.read(u8);
    reader.read(i16);
    reader.read(u16);
    reader.read(i32);
    reader.read(u32);
    reader.read(i64);
    reader.read(u64);
    reader.read(f32);
    reader.read(f64);
    reader.read(text);
    reader.read(id);

    REQUIRE(reader.ok());
    CHECK(flag == true);
    CHECK(i8 == -8);
    CHECK(u8 == 200);
    CHECK(i16 == -300);
    CHECK(u16 == 60000);
    CHECK(i32 == -70000);
    CHECK(u32 == 4000000000U);
    CHECK(i64 == -5000000000LL);
    CHECK(u64 == 18000000000000000000ULL);
    CHECK(f32 == 0.5f);
    CHECK(f64 == 0.25);
    CHECK(text == "TechEngine");
    CHECK(id == StringId{"TechEngine.Transform"});
    CHECK(reader.remaining() == 0);
}

TEST_CASE("a bulk span round-trips", "[core][serialization]") {
    const std::vector<Pixel> source = {Pixel{1, 2, 3, 4}, Pixel{5, 6, 7, 8}, Pixel{9, 10, 11, 12}};

    std::vector<std::byte> buffer;
    Writer writer{buffer};
    writer.writeSpan(std::span<const Pixel>{source});

    Reader reader{buffer};
    std::vector<Pixel> restored;
    reader.readSpan(restored);

    REQUIRE(reader.ok());
    CHECK(restored == source);
    CHECK(reader.remaining() == 0);
}

TEST_CASE("encoding the same input twice produces identical bytes", "[core][serialization]") {
    const auto encode = [] {
        std::vector<std::byte> buffer;
        Writer writer{buffer};
        writer.writeHeader();
        writer.write(std::string_view{"deterministic"});
        writer.write(StringId{"TechEngine.Transform"});
        writer.write(1.5);
        return buffer;
    };

    CHECK(encode() == encode());
}

TEST_CASE("a truncated buffer fails soft", "[core][serialization]") {
    std::vector<std::byte> buffer;
    Writer writer{buffer};
    writer.write(static_cast<std::uint64_t>(42));

    buffer.resize(buffer.size() - 1);

    Reader reader{buffer};
    std::uint64_t value = 0;
    reader.read(value);

    CHECK_FALSE(reader.ok());
    CHECK(reader.status() == ReadStatus::Truncated);
}

TEST_CASE("a corrupted length prefix fails soft instead of over-reading", "[core][serialization]") {
    std::vector<std::byte> buffer;
    Writer writer{buffer};
    writer.write(std::string_view{"short"});

    buffer[0] = std::byte{0xFF};
    buffer[1] = std::byte{0xFF};

    Reader reader{buffer};
    std::string text;
    reader.read(text);

    CHECK_FALSE(reader.ok());
    CHECK(reader.status() == ReadStatus::Truncated);
}

TEST_CASE("the first failure is the one reported", "[core][serialization]") {
    const std::vector<std::byte> empty;
    Reader reader{empty};

    BlobHeader header;
    reader.readHeader(header);
    REQUIRE(reader.status() == ReadStatus::Truncated);

    std::uint32_t value = 0;
    reader.read(value);

    CHECK(reader.status() == ReadStatus::Truncated);
}

TEST_CASE("a wrong magic is rejected", "[core][serialization]") {
    std::vector<std::byte> buffer;
    Writer writer{buffer};
    writer.writeHeader();

    buffer[0] = std::byte{0x00};

    Reader reader{buffer};
    BlobHeader header;
    reader.readHeader(header);

    CHECK_FALSE(reader.ok());
    CHECK(reader.status() == ReadStatus::BadMagic);
}

TEST_CASE("a wrong format version is rejected", "[core][serialization]") {
    std::vector<std::byte> buffer;
    Writer writer{buffer};
    writer.writeHeader();

    buffer[4] = std::byte{0xFE};
    buffer[5] = std::byte{0xFF};

    Reader reader{buffer};
    BlobHeader header;
    reader.readHeader(header);

    CHECK_FALSE(reader.ok());
    CHECK(reader.status() == ReadStatus::BadVersion);
}
