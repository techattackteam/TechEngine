#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>
#include <TechEngine/testing/ErrorCategoryChecks.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <system_error>
#include <vector>

using TechEngine::Reader;
using TechEngine::ReadError;
using TechEngine::readErrorCategory;
using TechEngine::Writer;

static constexpr std::array<ReadError, 3> ALL_CODES{ReadError::Truncated, ReadError::BadMagic, ReadError::BadVersion};

TEST_CASE("every ReadError code converts into the read category", "[core][serialization][readerror]") {
    TechEngineTests::checkErrorCategory<ReadError>(ALL_CODES, readErrorCategory(), "read");
}

TEST_CASE("a reader that never fails reports a default error_code", "[core][serialization][readerror]") {
    std::vector<std::byte> buffer;
    Writer writer{buffer};
    writer.write(std::uint32_t{7});

    Reader reader{buffer};
    std::uint32_t value = 0;
    reader.read(value);

    CHECK(reader.ok());
    CHECK(reader.error() == std::error_code{});
}
