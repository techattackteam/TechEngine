#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/platform/window/Window.hpp>
#include <TechEngine/testing/AssertCapture.hpp>
#include <TechEngine/testing/WindowTerminateGuard.hpp>

#include <render/MaterialBuffer.hpp>
#include <render/device/RenderDevice.hpp>

#include <catch2/catch_test_macros.hpp>
#include <glad/gl.h>

#include <cstddef>
#include <cstdint>
#include <set>
#include <system_error>
#include <thread>
#include <vector>

using TechEngine::DEFAULT_MATERIAL_RECORD;
using TechEngine::GpuMaterial;
using TechEngine::MaterialBuffer;
using TechEngine::Vec3;
using TechEngine::Vec4;

static constexpr std::uint32_t SMALL_CAPACITY = 4;

template<typename Body>
static bool runOnContext(TechEngine::Window& window, Body&& body) {
    bool loaded = false;
    {
        std::jthread worker([&] {
            window.makeContextCurrent();
            const TechEngine::GlProcLoader loader = window.processLoader();
            loaded = loader != nullptr && gladLoadGL(loader) != 0 && GLAD_GL_VERSION_4_6 != 0;
            if (loaded) {
                body();
            }
            window.releaseContext();
        });
    }
    return loaded;
}

static GpuMaterial recordWithBaseColor(const float red) {
    GpuMaterial record;
    record.albedo = Vec4{red, 0.0F, 0.0F, 1.0F};
    return record;
}

TEST_CASE("the first record handed out is 1, because record 0 is the default material's", "[client][render][material]") {
    MaterialBuffer materials{SMALL_CAPACITY};

    REQUIRE(materials.allocate() == 1);
    REQUIRE(materials.allocate() == 2);
    REQUIRE(materials.allocate() == 3);
    REQUIRE(materials.getCapacity() == SMALL_CAPACITY);
}

TEST_CASE("a full buffer doubles its capacity, and no record is handed out twice", "[client][render][material]") {
    MaterialBuffer materials{SMALL_CAPACITY};
    std::set<std::uint32_t> handedOut;
    std::vector<std::uint32_t> capacities;

    for (int i = 0; i < 15; i++) {
        const std::uint32_t record = materials.allocate();
        REQUIRE(record != DEFAULT_MATERIAL_RECORD);
        REQUIRE(record < materials.getCapacity());
        REQUIRE(handedOut.insert(record).second);
        capacities.push_back(materials.getCapacity());
    }

    REQUIRE(capacities[2] == 4);
    REQUIRE(capacities[3] == 8);
    REQUIRE(capacities[6] == 8);
    REQUIRE(capacities[7] == 16);
    REQUIRE(capacities[14] == 16);
}

TEST_CASE("a freed record is handed out again before an unused one, and before the buffer grows", "[client][render][material]") {
    MaterialBuffer materials{SMALL_CAPACITY};
    const std::uint32_t first = materials.allocate();
    const std::uint32_t second = materials.allocate();
    const std::uint32_t third = materials.allocate();

    materials.free(second);
    const std::uint32_t reused = materials.allocate();

    REQUIRE(reused == second);
    REQUIRE(reused != first);
    REQUIRE(reused != third);
    REQUIRE(materials.getCapacity() == SMALL_CAPACITY);
    REQUIRE(materials.allocate() == 4);
    REQUIRE(materials.getCapacity() == 2 * SMALL_CAPACITY);
}

TEST_CASE("freeing record 0, a record never handed out, or a record twice fires a check and changes nothing", "[client][render][material]") {
    const TechEngineTests::AssertHandlerGuard guard;
    MaterialBuffer materials{SMALL_CAPACITY};
    const std::uint32_t record = materials.allocate();

    materials.free(DEFAULT_MATERIAL_RECORD);
    REQUIRE(TechEngineTests::g_fired.size() == 1);
    materials.free(3);
    REQUIRE(TechEngineTests::g_fired.size() == 2);
    materials.free(record);
    REQUIRE(TechEngineTests::g_fired.size() == 2);
    materials.free(record);
    REQUIRE(TechEngineTests::g_fired.size() == 3);

    REQUIRE(materials.allocate() == record);
    REQUIRE(materials.allocate() == record + 1);
}

TEST_CASE("the default material is white, lit, fully rough and not metallic, with no textures", "[client][render][material]") {
    const GpuMaterial material = MaterialBuffer::getDefaultMaterial();

    REQUIRE(material.albedo == Vec4{1.0F, 1.0F, 1.0F, 1.0F});
    REQUIRE(material.emissiveFactor == Vec3{0.0F, 0.0F, 0.0F});
    REQUIRE(material.occlusionStrength == 1.0F);
    REQUIRE(material.roughnessFactor == 1.0F);
    REQUIRE(material.metallicFactor == 0.0F);
    REQUIRE(material.normalScale == 1.0F);
    REQUIRE(material.shadingModel == 0);
    for (const std::uint32_t word: material.textureHandles) {
        REQUIRE(word == 0);
    }
}

TEST_CASE("setTextureHandle writes one input's handle low word first and leaves the other inputs alone", "[client][render][material]") {
    GpuMaterial material;

    material.setTextureHandle(2, 0x1122334455667788ULL);

    REQUIRE(material.textureHandles[4] == 0x55667788U);
    REQUIRE(material.textureHandles[5] == 0x11223344U);
    for (const std::size_t word: {0U, 1U, 2U, 3U, 6U, 7U}) {
        REQUIRE(material.textureHandles[word] == 0);
    }
}

TEST_CASE("initialize uploads record 0, and growing past the capacity replaces the buffer and keeps every record", "[client][render][material][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Material buffer growth test") == std::error_code{});

    std::error_code initializeError;
    std::vector<std::error_code> writeErrors;
    bool firstBufferValid = false;
    bool bufferReplaced = false;
    std::size_t liveAfterGrowth = 0;
    std::uint32_t capacityAfterGrowth = 0;
    std::vector<GpuMaterial> kept;
    GpuMaterial defaultRecord;
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runOnContext(window, [&] {
        TechEngine::RenderDevice device;
        MaterialBuffer materials{SMALL_CAPACITY};
        initializeError = materials.initialize(device);
        const TechEngine::BufferHandle firstBuffer = materials.getBuffer();
        firstBufferValid = firstBuffer.valid();
        for (int i = 1; i <= 4; i++) {
            const std::uint32_t record = materials.allocate();
            writeErrors.push_back(materials.write(record, recordWithBaseColor(static_cast<float>(i) / 8.0F)));
        }
        bufferReplaced = materials.getBuffer().valid() && materials.getBuffer() != firstBuffer;
        liveAfterGrowth = device.getLiveObjectCount();
        capacityAfterGrowth = materials.getCapacity();
        for (std::uint32_t record = 1; record <= 4; record++) {
            kept.push_back(materials.getRecord(record));
        }
        defaultRecord = materials.getRecord(DEFAULT_MATERIAL_RECORD);
        materials.shutdown();
        device.shutdown();
        error = glGetError();
    });

    REQUIRE(loaded);
    REQUIRE(initializeError == std::error_code{});
    CHECK(firstBufferValid);
    for (const std::error_code& writeError: writeErrors) {
        CHECK(writeError == std::error_code{});
    }
    CHECK(bufferReplaced);
    CHECK(liveAfterGrowth == 1);
    CHECK(capacityAfterGrowth == 2 * SMALL_CAPACITY);
    REQUIRE(kept.size() == 4);
    for (std::size_t i = 0; i < kept.size(); i++) {
        CHECK(kept[i] == recordWithBaseColor(static_cast<float>(i + 1) / 8.0F));
    }
    CHECK(defaultRecord == MaterialBuffer::getDefaultMaterial());
    CHECK(error == GL_NO_ERROR);
}
