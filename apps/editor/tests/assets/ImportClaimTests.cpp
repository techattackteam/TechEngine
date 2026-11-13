#include <assets/ImportClaim.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <string_view>
#include <thread>

using TechEngine::ImportClaim;

static constexpr std::string_view KIT_FOLDER = "project://assets/client/Kit";
static constexpr std::string_view SKY_TEXTURE = "project://assets/client/Sky.tetexture";
static constexpr std::chrono::milliseconds BLOCKED_WAIT{100};
static constexpr std::chrono::seconds CLAIM_TIMEOUT{5};

TEST_CASE("a claim on a held path waits until the holder releases it", "[editor][assets][claim]") {
    std::atomic<bool> claimed = false;
    std::future<void> second;
    {
        const ImportClaim first(KIT_FOLDER);
        second = std::async(std::launch::async, [&claimed] {
            const ImportClaim claim(KIT_FOLDER);
            claimed = true;
        });
        std::this_thread::sleep_for(BLOCKED_WAIT);
        CHECK_FALSE(claimed);
    }

    REQUIRE(second.wait_for(CLAIM_TIMEOUT) == std::future_status::ready);
    CHECK(claimed);
}

TEST_CASE("a claim on another path does not wait, and a released path can be claimed again", "[editor][assets][claim]") {
    std::future<void> other;
    const ImportClaim first(KIT_FOLDER);

    other = std::async(std::launch::async, [] {
        const ImportClaim sky(SKY_TEXTURE);
    });
    REQUIRE(other.wait_for(CLAIM_TIMEOUT) == std::future_status::ready);

    other = std::async(std::launch::async, [] {
        const ImportClaim sky(SKY_TEXTURE);
    });
    REQUIRE(other.wait_for(CLAIM_TIMEOUT) == std::future_status::ready);
}
