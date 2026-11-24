#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/texture/Texture.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/import/ImageTestFiles.hpp>
#include <assets/import/TextureImporter.hpp>
#include <assets/index/AssetIndex.hpp>
#include <project/ProjectError.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iterator>
#include <latch>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

using TechEngine::ProjectError;
using TechEngine::ResourceId;
using TechEngine::SkyImportRequest;
using TechEngine::SkyImportResult;
using TechEngine::Texture;
using TechEngine::TextureImporter;
using TechEngineTests::ScratchDirectory;

static constexpr std::uint64_t FIRST_TEST_ID = 0x2000;
static constexpr std::string_view SKY_SOURCE = "project://assets/client/Sky.hdr";
static constexpr std::string_view SKY_TEXTURE = "project://assets/client/Sky.tetexture";
static constexpr std::string_view SKY_TEXTURE_1 = "project://assets/client/Sky_1.tetexture";
static constexpr std::uint32_t RACE_ATTEMPTS = 8;

class SkyImportFixture {
public:
    ScratchDirectory scratch;
    TechEngine::MountTable mounts;
    TechEngine::FileAccess files{mounts};
    std::uint64_t nextId = FIRST_TEST_ID;

    explicit SkyImportFixture(const std::string_view name) : scratch(name) {
        mounts.mount("project", scratch.root());
        scratch.makeDirectory("assets/client");
    }

    std::error_code importSky(const std::string_view sourcePath, SkyImportResult& out, const std::string_view name = {}) {
        SkyImportRequest request;
        request.sourcePath = std::string(sourcePath);
        request.name = std::string(name);
        const TextureImporter importer(files, [this] {
            return ResourceId{nextId++};
        });
        return importer.importSky(request, out);
    }

    Texture readTexture(const std::string_view path) const {
        std::vector<std::byte> bytes;
        REQUIRE(files.read(path, bytes) == std::error_code{});
        Texture texture;
        REQUIRE(texture.read(bytes) == std::error_code{});
        return texture;
    }

    bool exists(const std::string_view relative) const {
        return std::filesystem::exists(scratch.root() / relative);
    }
};

static void unpackPixel(const Texture& texture, const std::uint32_t level, const std::size_t pixel, float& red, float& green, float& blue) {
    const std::vector<std::byte>& bytes = texture.mips[level];
    std::uint32_t packed = 0;
    for (std::size_t i = 0; i < 4; i++) {
        packed |= static_cast<std::uint32_t>(bytes[pixel * 4 + i]) << (8 * i);
    }
    TextureImporter::unpackRgb9E5(packed, red, green, blue);
}

TEST_CASE("an 8x4 Radiance file becomes an RGB9E5 texture of its size with the full mip chain", "[editor][assets][image][os]") {
    SkyImportFixture fixture{"imageImportSky"};
    fixture.scratch.writeFile("assets/client/Sky.hdr", TechEngineTests::gradientRadianceFile(8, 4));
    SkyImportResult result;

    REQUIRE(fixture.importSky(SKY_SOURCE, result) == std::error_code{});

    const Texture texture = fixture.readTexture(SKY_TEXTURE);
    CHECK(texture.format == TechEngine::TextureFormat::Rgb9E5);
    CHECK(texture.shape == TechEngine::TextureShape::Texture2D);
    CHECK(texture.width == 8);
    CHECK(texture.height == 4);
    REQUIRE(texture.mips.size() == 4);
    CHECK(texture.mips.size() == Texture::mipCount(8, 4));
    for (std::uint32_t level = 0; level < texture.mips.size(); level++) {
        CHECK(texture.mips[level].size() == Texture::mipByteSize(TechEngine::TextureFormat::Rgb9E5, 8, 4, level));
    }

    for (std::uint32_t y = 0; y < 4; y++) {
        for (std::uint32_t x = 0; x < 8; x++) {
            const TechEngineTests::RgbePixel source = TechEngineTests::gradientPixel(x, y);
            const float expected[3] = {TechEngineTests::rgbeChannel(source.red, source.exponent), TechEngineTests::rgbeChannel(source.green, source.exponent), TechEngineTests::rgbeChannel(source.blue, source.exponent)};
            const float margin = std::max({expected[0], expected[1], expected[2]}) / 256.0f;
            float red = 0.0f;
            float green = 0.0f;
            float blue = 0.0f;
            unpackPixel(texture, 0, y * 8 + x, red, green, blue);

            CHECK(red == Catch::Approx(expected[0]).margin(margin));
            CHECK(green == Catch::Approx(expected[1]).margin(margin));
            CHECK(blue == Catch::Approx(expected[2]).margin(margin));
        }
    }

    SECTION("the last level is the mean of the whole image") {
        float sums[3] = {0.0f, 0.0f, 0.0f};
        for (std::uint32_t y = 0; y < 4; y++) {
            for (std::uint32_t x = 0; x < 8; x++) {
                const TechEngineTests::RgbePixel source = TechEngineTests::gradientPixel(x, y);
                sums[0] += TechEngineTests::rgbeChannel(source.red, source.exponent) / 32.0f;
                sums[1] += TechEngineTests::rgbeChannel(source.green, source.exponent) / 32.0f;
                sums[2] += TechEngineTests::rgbeChannel(source.blue, source.exponent) / 32.0f;
            }
        }
        const float margin = std::max({sums[0], sums[1], sums[2]}) / 128.0f;
        float red = 0.0f;
        float green = 0.0f;
        float blue = 0.0f;
        unpackPixel(texture, 3, 0, red, green, blue);

        CHECK(red == Catch::Approx(sums[0]).margin(margin));
        CHECK(green == Catch::Approx(sums[1]).margin(margin));
        CHECK(blue == Catch::Approx(sums[2]).margin(margin));
    }

    SECTION("the sampler repeats in U, clamps in V and filters linearly") {
        CHECK(texture.sampler.wrapU == TechEngine::TextureWrap::Repeat);
        CHECK(texture.sampler.wrapV == TechEngine::TextureWrap::Clamp);
        CHECK(texture.sampler.filter == TechEngine::TextureFilter::Linear);
    }

    SECTION("the import block names the source, has the empty key and the default settings") {
        CHECK(texture.importBlock.sourcePath == "assets/client/Sky.hdr");
        CHECK(texture.importBlock.sourceIndex == 0);
        CHECK(texture.importBlock.sourceName.empty());
        CHECK(texture.importBlock.settings == TechEngine::ImportSettings{});
    }

    SECTION("the texture has a fresh id and the result holds its one record") {
        CHECK(texture.id == ResourceId{FIRST_TEST_ID});
        REQUIRE(result.records.size() == 1);
        CHECK(result.records[0].id == texture.id);
        CHECK(result.records[0].typeTag == TechEngine::TEXTURE_TYPE_TAG);
        CHECK(result.records[0].side == TechEngine::Side::Client);
        CHECK(result.records[0].path == SKY_TEXTURE);
        CHECK(fixture.nextId == FIRST_TEST_ID + 1);
    }

    SECTION("the output is beside the source, with no folder") {
        CHECK(fixture.exists("assets/client/Sky.tetexture"));
        CHECK(fixture.exists("assets/client/Sky.hdr"));
        CHECK_FALSE(fixture.exists("assets/client/Sky"));
    }
}

TEST_CASE("a file that holds the output's name is left as it was, and the sky takes the next free name", "[editor][assets][image][os]") {
    SkyImportFixture fixture{"imageImportExists"};
    fixture.scratch.writeFile("assets/client/Sky.hdr", TechEngineTests::gradientRadianceFile(8, 4));
    fixture.scratch.writeFile("assets/client/Sky.tetexture", "kept");
    SkyImportResult result;

    SECTION("the base name alone") {
        REQUIRE(fixture.importSky(SKY_SOURCE, result) == std::error_code{});

        REQUIRE(result.records.size() == 1);
        CHECK(result.records[0].path == SKY_TEXTURE_1);
    }

    SECTION("the base name and the first suffix, so the second suffix is used") {
        fixture.scratch.writeFile("assets/client/Sky_1.tetexture", "kept too");

        REQUIRE(fixture.importSky(SKY_SOURCE, result) == std::error_code{});

        REQUIRE(result.records.size() == 1);
        CHECK(result.records[0].path == "project://assets/client/Sky_2.tetexture");
    }

    SECTION("a folder with the output's first free name") {
        fixture.scratch.makeDirectory("assets/client/Sky_1.tetexture");

        REQUIRE(fixture.importSky(SKY_SOURCE, result) == std::error_code{});

        REQUIRE(result.records.size() == 1);
        CHECK(result.records[0].path == "project://assets/client/Sky_2.tetexture");
    }

    std::vector<std::byte> bytes;
    REQUIRE(fixture.files.read(SKY_TEXTURE, bytes) == std::error_code{});
    CHECK(bytes.size() == 4);
    CHECK(fixture.readTexture(result.records[0].path).id == result.records[0].id);
}

TEST_CASE("a sky created twice from one source gives two textures with different ids", "[editor][assets][image][os]") {
    SkyImportFixture fixture{"imageImportTwice"};
    fixture.scratch.writeFile("assets/client/Sky.hdr", TechEngineTests::gradientRadianceFile(8, 4));
    SkyImportResult first;
    REQUIRE(fixture.importSky(SKY_SOURCE, first) == std::error_code{});
    std::vector<std::byte> before;
    REQUIRE(fixture.files.read(SKY_TEXTURE, before) == std::error_code{});

    SkyImportResult second;
    REQUIRE(fixture.importSky(SKY_SOURCE, second) == std::error_code{});

    REQUIRE(first.records.size() == 1);
    REQUIRE(second.records.size() == 1);
    CHECK(first.records[0].path == SKY_TEXTURE);
    CHECK(second.records[0].path == SKY_TEXTURE_1);
    CHECK(first.records[0].id != second.records[0].id);
    CHECK(fixture.readTexture(SKY_TEXTURE_1).id == second.records[0].id);
    std::vector<std::byte> after;
    REQUIRE(fixture.files.read(SKY_TEXTURE, after) == std::error_code{});
    CHECK(after == before);
}

TEST_CASE("of two creates of one sky at once, both succeed, each into its own file", "[editor][assets][image][race][os]") {
    SkyImportFixture fixture{"imageImportRace"};
    fixture.scratch.writeFile("assets/client/Sky.hdr", TechEngineTests::gradientRadianceFile(256, 128));
    std::atomic<std::uint64_t> nextId = FIRST_TEST_ID;
    const TextureImporter importer(fixture.files, [&nextId] {
        return ResourceId{nextId++};
    });

    for (std::uint32_t attempt = 0; attempt < RACE_ATTEMPTS; attempt++) {
        std::filesystem::remove(fixture.scratch.root() / "assets/client/Sky.tetexture");
        std::filesystem::remove(fixture.scratch.root() / "assets/client/Sky_1.tetexture");
        std::latch start(2);
        std::array<SkyImportResult, 2> results;
        std::array<std::error_code, 2> errors;
        const auto run = [&](const std::size_t index) {
            SkyImportRequest request;
            request.sourcePath = std::string(SKY_SOURCE);
            start.arrive_and_wait();
            errors[index] = importer.importSky(request, results[index]);
        };
        std::thread first(run, 0);
        std::thread second(run, 1);
        first.join();
        second.join();

        REQUIRE(std::ranges::count(errors, std::error_code{}) == 2);
        REQUIRE(results[0].records.size() == 1);
        REQUIRE(results[1].records.size() == 1);
        const bool firstTookBase = results[0].records[0].path == SKY_TEXTURE;
        const SkyImportResult& base = firstTookBase ? results[0] : results[1];
        const SkyImportResult& suffixed = firstTookBase ? results[1] : results[0];
        CHECK(base.records[0].path == SKY_TEXTURE);
        CHECK(suffixed.records[0].path == SKY_TEXTURE_1);
        CHECK(fixture.readTexture(SKY_TEXTURE).id == base.records[0].id);
        CHECK(fixture.readTexture(SKY_TEXTURE_1).id == suffixed.records[0].id);
        CHECK(base.records[0].id != suffixed.records[0].id);
        CHECK(std::distance(std::filesystem::directory_iterator{fixture.scratch.root() / "assets/client"}, std::filesystem::directory_iterator{}) == 3);
    }
}

TEST_CASE("a path that is not a .hdr is refused before anything is read or written", "[editor][assets][image][os]") {
    SkyImportFixture fixture{"imageImportWrongType"};
    fixture.scratch.writeFile("assets/client/Sky.png", "not read");
    fixture.scratch.writeFile("assets/client/Sky", "no extension");
    fixture.scratch.writeFile("assets/client/Sky.hdr.txt", "wrong last extension");
    SkyImportResult result;

    CHECK(fixture.importSky("project://assets/client/Sky.png", result) == ProjectError::UnsupportedImageType);
    CHECK(fixture.importSky("project://assets/client/Sky", result) == ProjectError::UnsupportedImageType);
    CHECK(fixture.importSky("project://assets/client/Sky.hdr.txt", result) == ProjectError::UnsupportedImageType);
    CHECK(fixture.importSky("project://assets/client/Missing.png", result) == ProjectError::UnsupportedImageType);

    CHECK_FALSE(fixture.exists("assets/client/Sky.tetexture"));
    CHECK_FALSE(fixture.exists("assets/client/Sky.png.tetexture"));
    CHECK(fixture.nextId == FIRST_TEST_ID);
    CHECK(result.records.empty());
}

TEST_CASE("a missing source is refused with the file error and writes nothing", "[editor][assets][image][os]") {
    SkyImportFixture fixture{"imageImportMissing"};
    SkyImportResult result;

    CHECK(fixture.importSky(SKY_SOURCE, result) == TechEngine::FileError::NotFound);

    CHECK_FALSE(fixture.exists("assets/client/Sky.tetexture"));
}

TEST_CASE("a .hdr that is not a Radiance file is refused and writes nothing", "[editor][assets][image][os]") {
    SkyImportFixture fixture{"imageImportGarbage"};
    fixture.scratch.writeFile("assets/client/Sky.hdr", "definitely not radiance");
    fixture.scratch.writeFile("assets/client/Empty.hdr", "");
    SkyImportResult result;

    CHECK(fixture.importSky(SKY_SOURCE, result) == ProjectError::ImageUnreadable);
    CHECK(fixture.importSky("project://assets/client/Empty.hdr", result) == ProjectError::ImageUnreadable);

    CHECK_FALSE(fixture.exists("assets/client/Sky.tetexture"));
    CHECK_FALSE(fixture.exists("assets/client/Empty.tetexture"));
}

TEST_CASE("a source outside a side folder of the assets folder is refused", "[editor][assets][image][os]") {
    SkyImportFixture fixture{"imageImportOutsideAssets"};
    fixture.scratch.writeFile("Sky.hdr", TechEngineTests::gradientRadianceFile(8, 4));
    fixture.scratch.writeFile("assets/Sky.hdr", TechEngineTests::gradientRadianceFile(8, 4));
    fixture.scratch.writeFile("assets/other/Sky.hdr", TechEngineTests::gradientRadianceFile(8, 4));
    SkyImportResult result;

    CHECK(fixture.importSky("project://Sky.hdr", result) == ProjectError::SourceNotInAssets);
    CHECK(fixture.importSky("project://assets/Sky.hdr", result) == ProjectError::SourceNotInAssets);
    CHECK(fixture.importSky("project://assets/other/Sky.hdr", result) == ProjectError::SourceNotInAssets);

    CHECK_FALSE(fixture.exists("Sky.tetexture"));
    CHECK_FALSE(fixture.exists("assets/other/Sky.tetexture"));
}

TEST_CASE("a source on an alias that is not mounted is refused with the file error and writes nothing", "[editor][assets][image][os]") {
    SkyImportFixture fixture{"imageImportNoMount"};
    fixture.scratch.writeFile("assets/client/Sky.hdr", TechEngineTests::gradientRadianceFile(8, 4));
    SkyImportResult result;

    CHECK(fixture.importSky("elsewhere://assets/client/Sky.hdr", result) == TechEngine::FileError::NoMount);

    CHECK(result.records.empty());
    CHECK_FALSE(fixture.exists("assets/client/Sky.tetexture"));
    CHECK(fixture.nextId == FIRST_TEST_ID);
}

TEST_CASE("the record's side follows the side folder and the extension is read without case", "[editor][assets][image][os]") {
    SkyImportFixture fixture{"imageImportSides"};
    fixture.scratch.writeFile("assets/server/Dusk.HDR", TechEngineTests::gradientRadianceFile(8, 4));
    fixture.scratch.writeFile("assets/common/Dawn.hdr", TechEngineTests::gradientRadianceFile(8, 4));
    SkyImportResult server;
    SkyImportResult common;

    REQUIRE(fixture.importSky("project://assets/server/Dusk.HDR", server) == std::error_code{});
    REQUIRE(fixture.importSky("project://assets/common/Dawn.hdr", common) == std::error_code{});

    REQUIRE(server.records.size() == 1);
    CHECK(server.records[0].side == TechEngine::Side::Server);
    CHECK(server.records[0].path == "project://assets/server/Dusk.tetexture");
    REQUIRE(common.records.size() == 1);
    CHECK(common.records[0].side == TechEngine::Side::Common);
    CHECK(common.records[0].id == ResourceId{FIRST_TEST_ID + 1});
}

TEST_CASE("odd and tiny sizes get the full chain", "[editor][assets][image][os]") {
    SkyImportFixture fixture{"imageImportOddSizes"};
    fixture.scratch.writeFile("assets/client/Odd.hdr", TechEngineTests::gradientRadianceFile(5, 3));
    fixture.scratch.writeFile("assets/client/One.hdr", TechEngineTests::gradientRadianceFile(1, 1));
    fixture.scratch.writeFile("assets/client/Strip.hdr", TechEngineTests::gradientRadianceFile(1, 6));
    SkyImportResult result;

    REQUIRE(fixture.importSky("project://assets/client/Odd.hdr", result) == std::error_code{});
    REQUIRE(fixture.importSky("project://assets/client/One.hdr", result) == std::error_code{});
    REQUIRE(fixture.importSky("project://assets/client/Strip.hdr", result) == std::error_code{});

    const Texture odd = fixture.readTexture("project://assets/client/Odd.tetexture");
    CHECK(odd.width == 5);
    CHECK(odd.height == 3);
    CHECK(odd.mips.size() == 3);
    CHECK(odd.mips.back().size() == 4);

    const Texture one = fixture.readTexture("project://assets/client/One.tetexture");
    CHECK(one.mips.size() == 1);
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    unpackPixel(one, 0, 0, red, green, blue);
    const TechEngineTests::RgbePixel source = TechEngineTests::gradientPixel(0, 0);
    CHECK(red == Catch::Approx(TechEngineTests::rgbeChannel(source.red, source.exponent)).margin(0.01f));

    const Texture strip = fixture.readTexture("project://assets/client/Strip.tetexture");
    CHECK(strip.mips.size() == 3);
}

TEST_CASE("the output path replaces the last extension of the file name", "[editor][assets][image]") {
    CHECK(TextureImporter::skyOutputPath("project://assets/client/Sky.hdr", "", 0) == "project://assets/client/Sky.tetexture");
    CHECK(TextureImporter::skyOutputPath("project://assets/client/Sky.v2.hdr", "", 0) == "project://assets/client/Sky.v2.tetexture");
    CHECK(TextureImporter::skyOutputPath("project://assets/client/Sky", "", 0) == "project://assets/client/Sky.tetexture");
    CHECK(TextureImporter::skyOutputPath("project://assets/my.folder/Sky", "", 0) == "project://assets/my.folder/Sky.tetexture");
}

TEST_CASE("a typed sky name replaces the source's name beside the source", "[editor][assets][image]") {
    CHECK(TextureImporter::skyOutputPath("project://assets/client/Sky.hdr", "Dusk", 0) == "project://assets/client/Dusk.tetexture");
    CHECK(TextureImporter::skyOutputPath("project://assets/client/Sky.hdr", "Dusk", 3) == "project://assets/client/Dusk_3.tetexture");
    CHECK(TextureImporter::skyOutputPath("project://assets/my.folder/Sky.hdr", "Dusk.v2", 1) == "project://assets/my.folder/Dusk.v2_1.tetexture");
}

TEST_CASE("a sky created under a typed name writes that file, and the same name again takes a suffix", "[editor][assets][image][os]") {
    SkyImportFixture fixture{"skyTypedName"};
    fixture.scratch.writeFile("assets/client/Sky.hdr", TechEngineTests::gradientRadianceFile(8, 4));

    SkyImportResult first;
    REQUIRE(fixture.importSky(SKY_SOURCE, first, "Dusk") == std::error_code{});
    SkyImportResult second;
    REQUIRE(fixture.importSky(SKY_SOURCE, second, "Dusk") == std::error_code{});

    REQUIRE(first.records.size() == 1);
    REQUIRE(second.records.size() == 1);
    CHECK(first.records[0].path == "project://assets/client/Dusk.tetexture");
    CHECK(second.records[0].path == "project://assets/client/Dusk_1.tetexture");
    CHECK(first.records[0].id != second.records[0].id);
    CHECK_FALSE(std::filesystem::exists(fixture.scratch.root() / "assets" / "client" / "Sky.tetexture"));
}

TEST_CASE("a suffix goes between the stem and the extension", "[editor][assets][image]") {
    CHECK(TextureImporter::skyOutputPath("project://assets/client/Sky.hdr", "", 1) == "project://assets/client/Sky_1.tetexture");
    CHECK(TextureImporter::skyOutputPath("project://assets/client/Sky.hdr", "", 12) == "project://assets/client/Sky_12.tetexture");
    CHECK(TextureImporter::skyOutputPath("project://assets/client/Sky.v2.hdr", "", 1) == "project://assets/client/Sky.v2_1.tetexture");
    CHECK(TextureImporter::skyOutputPath("project://assets/my.folder/Sky", "", 1) == "project://assets/my.folder/Sky_1.tetexture");
}
