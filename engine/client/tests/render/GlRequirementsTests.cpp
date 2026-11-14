#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/StoredLogRecord.hpp>
#include <TechEngine/testing/LogCapture.hpp>

#include <render/GlRequirements.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string>
#include <string_view>

static constexpr int LOADED_GL_46 = 40006;
static constexpr std::string_view VERSION_REQUIREMENT = "4.6";
static constexpr std::string_view BINDLESS_REQUIREMENT = "ARB_bindless_texture";

static std::size_t errorCount(const TechEngineTests::LogCaptureGuard& logs) {
    std::size_t count = 0;
    for (const TechEngine::StoredLogRecord& record: logs.getRecords()) {
        if (record.level == TechEngine::Level::Error) {
            count++;
        }
    }
    return count;
}

static bool errorNames(const TechEngineTests::LogCaptureGuard& logs, const std::string_view text) {
    for (const TechEngine::StoredLogRecord& record: logs.getRecords()) {
        if (record.level == TechEngine::Level::Error && record.message.find(text) != std::string::npos) {
            return true;
        }
    }
    return false;
}

TEST_CASE("The startup check passes a 4.6 core context with bindless textures and logs no error", "[client][render]") {
    const TechEngineTests::LogCaptureGuard logs;
    CHECK(TechEngine::meetsGlRequirements({.loadedVersion = LOADED_GL_46, .core46 = true, .bindlessTextures = true}));
    CHECK(errorCount(logs) == 0);
}

TEST_CASE("The startup check logs that GL 4.6 is the requirement that failed", "[client][render]") {
    const TechEngineTests::LogCaptureGuard logs;
    CHECK_FALSE(TechEngine::meetsGlRequirements({.loadedVersion = 40005, .core46 = false, .bindlessTextures = true}));
    CHECK(errorNames(logs, VERSION_REQUIREMENT));
}

TEST_CASE("The startup check logs that ARB_bindless_texture is the requirement that failed", "[client][render]") {
    const TechEngineTests::LogCaptureGuard logs;
    CHECK_FALSE(TechEngine::meetsGlRequirements({.loadedVersion = LOADED_GL_46, .core46 = true, .bindlessTextures = false}));
    CHECK(errorNames(logs, BINDLESS_REQUIREMENT));
}

TEST_CASE("The startup check logs both requirements when both failed", "[client][render]") {
    const TechEngineTests::LogCaptureGuard logs;
    CHECK_FALSE(TechEngine::meetsGlRequirements({.loadedVersion = 40005, .core46 = false, .bindlessTextures = false}));
    CHECK(errorNames(logs, VERSION_REQUIREMENT));
    CHECK(errorNames(logs, BINDLESS_REQUIREMENT));
}

TEST_CASE("The startup check refuses a context whose GL functions did not load", "[client][render]") {
    const TechEngineTests::LogCaptureGuard logs;
    CHECK_FALSE(TechEngine::meetsGlRequirements({.loadedVersion = 0, .core46 = false, .bindlessTextures = false}));
    CHECK(errorCount(logs) >= 1);
}
