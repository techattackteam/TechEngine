#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/client/render/RenderView.hpp>

#include <render/ViewSlot.hpp>

#include <catch2/catch_test_macros.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

using TechEngine::Mat4;
using TechEngine::Vec3;
using TechEngine::VIEW_TEXTURE_TOKEN_BIT;
using TechEngine::ViewCameraSource;
using TechEngine::ViewRequest;
using TechEngine::ViewSlot;

static ViewRequest requestOf(const std::uint64_t index, const int width, const int height, const bool visible = true) {
    ViewRequest request;
    request.token = VIEW_TEXTURE_TOKEN_BIT | index;
    request.width = width;
    request.height = height;
    request.visible = visible;
    return request;
}

TEST_CASE("view slot reads nothing before the first write and leaves its target alone", "[client][view]") {
    ViewSlot slot;
    std::vector<ViewRequest> out{requestOf(9, 1, 1)};

    CHECK_FALSE(slot.read(out));
    REQUIRE(out.size() == 1);
    CHECK(out.front().token == (VIEW_TEXTURE_TOKEN_BIT | 9));
}

TEST_CASE("view slot returns the latest write, and a read does not consume it", "[client][view]") {
    ViewSlot slot;
    std::vector<ViewRequest> out;
    const std::vector<ViewRequest> first{requestOf(1, 640, 360), requestOf(2, 320, 180, false)};
    const std::vector<ViewRequest> second{requestOf(1, 800, 600)};

    slot.write(first);
    slot.write(second);

    REQUIRE(slot.read(out));
    REQUIRE(out.size() == 1);
    CHECK(out.front().token == (VIEW_TEXTURE_TOKEN_BIT | 1));
    CHECK(out.front().width == 800);
    CHECK(out.front().height == 600);

    out.clear();
    REQUIRE(slot.read(out));
    REQUIRE(out.size() == 1);
    CHECK(out.front().width == 800);
}

TEST_CASE("view slot treats a write of no views as a write", "[client][view]") {
    ViewSlot slot;
    std::vector<ViewRequest> out{requestOf(1, 1, 1)};

    slot.write({});

    REQUIRE(slot.read(out));
    CHECK(out.empty());
}

TEST_CASE("view slot carries every field of a request, the pose included", "[client][view]") {
    ViewSlot slot;
    ViewRequest request = requestOf(3, 1920, 1080, false);
    request.cameraSource = ViewCameraSource::Pose;
    request.pose.world = glm::translate(Mat4(1.0f), Vec3{1.0f, 2.0f, 3.0f});
    request.pose.settings.verticalFieldOfView = 70.0f;
    request.pose.settings.nearPlane = 0.25f;
    request.pose.settings.farPlane = 500.0f;
    const std::vector<ViewRequest> written{request};
    std::vector<ViewRequest> out;

    slot.write(written);

    REQUIRE(slot.read(out));
    REQUIRE(out.size() == 1);
    const ViewRequest& read = out.front();
    CHECK(read.token == request.token);
    CHECK(read.width == 1920);
    CHECK(read.height == 1080);
    CHECK_FALSE(read.visible);
    CHECK(read.cameraSource == ViewCameraSource::Pose);
    const bool worldKept = read.pose.world == request.pose.world;
    CHECK(worldKept);
    CHECK(read.pose.settings.verticalFieldOfView == 70.0f);
    CHECK(read.pose.settings.nearPlane == 0.25f);
    CHECK(read.pose.settings.farPlane == 500.0f);
}

TEST_CASE("view slot reset goes back to never written, and the next write is read", "[client][view]") {
    ViewSlot slot;
    std::vector<ViewRequest> out;
    const std::vector<ViewRequest> before{requestOf(1, 10, 10)};
    const std::vector<ViewRequest> after{requestOf(2, 20, 20)};

    slot.write(before);
    slot.reset();

    CHECK_FALSE(slot.read(out));
    CHECK(out.empty());

    slot.write(after);
    REQUIRE(slot.read(out));
    REQUIRE(out.size() == 1);
    CHECK(out.front().token == (VIEW_TEXTURE_TOKEN_BIT | 2));
}

TEST_CASE("view slot never tears a write while render reads", "[client][view]") {
    ViewSlot slot;
    std::atomic<bool> done = false;
    std::atomic<bool> torn = false;

    std::jthread reader([&] {
        std::vector<ViewRequest> out;
        while (!done.load()) {
            if (!slot.read(out)) {
                continue;
            }
            for (const ViewRequest& request: out) {
                const int index = static_cast<int>(request.token & ~VIEW_TEXTURE_TOKEN_BIT);
                if (request.width != index || request.height != index * 2 || out.size() != 2) {
                    torn.store(true);
                }
            }
        }
    });

    std::vector<ViewRequest> requests(2);
    for (int i = 1; i <= 2000; i++) {
        requests[0] = requestOf(static_cast<std::uint64_t>(i), i, i * 2);
        requests[1] = requestOf(static_cast<std::uint64_t>(i), i, i * 2);
        slot.write(requests);
    }
    done.store(true);
    reader.join();

    CHECK_FALSE(torn.load());
}
