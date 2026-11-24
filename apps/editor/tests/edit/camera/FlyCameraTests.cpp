#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/camera/Camera.hpp>

#include <edit/camera/FlyCamera.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include <cmath>

using Catch::Approx;
using TechEngine::FlyCamera;
using TechEngine::FlyCameraInput;
using TechEngine::Mat4;
using TechEngine::Vec2;
using TechEngine::Vec3;

static constexpr float FRAME = 1.0f / 60.0f;
static constexpr float TOLERANCE = 0.0001f;
static constexpr float VIEW_HEIGHT = 500.0f;

static FlyCameraInput looking() {
    FlyCameraInput input;
    input.looking = true;
    return input;
}

static FlyCameraInput zooming(const float steps) {
    FlyCameraInput input;
    input.zoom = steps;
    return input;
}

static void run(FlyCamera& camera, const FlyCameraInput& input, const float seconds, const float step = FRAME) {
    const int frames = static_cast<int>(std::lround(seconds / step));
    for (int i = 0; i < frames; i++) {
        camera.update(input, step);
    }
}

static void turnTo(FlyCamera& camera, const float yawDegrees) {
    FlyCameraInput input = looking();
    input.lookDelta = Vec2(-glm::radians(yawDegrees) / FlyCamera::LOOK_RADIANS_PER_PIXEL, 0.0f);
    camera.update(input, FRAME);
    run(camera, looking(), 1.0f);
}

static void checkNear(const Vec3& actual, const Vec3& expected, const float margin = TOLERANCE) {
    CHECK(actual.x == Approx(expected.x).margin(margin));
    CHECK(actual.y == Approx(expected.y).margin(margin));
    CHECK(actual.z == Approx(expected.z).margin(margin));
}

TEST_CASE("the fly camera starts at the scene view's old fixed pose", "[editor][edit]") {
    const FlyCamera camera;
    const Mat4 position = glm::translate(Mat4(1.0f), Vec3(0.0f, 2.0f, 5.0f));
    const Mat4 expected = glm::rotate(position, glm::radians(-10.0f), Vec3(1.0f, 0.0f, 0.0f));
    const Mat4 world = camera.getWorld();
    for (int column = 0; column < 4; column++) {
        for (int row = 0; row < 4; row++) {
            CHECK(world[column][row] == Approx(expected[column][row]).margin(TOLERANCE));
        }
    }
}

TEST_CASE("the world matrix sits at the position and looks down its local -Z along the forward", "[editor][edit]") {
    FlyCamera camera;
    turnTo(camera, 37.0f);
    camera.update(zooming(2.0f), FRAME);
    run(camera, FlyCameraInput{}, 0.1f);

    const Mat4 world = camera.getWorld();
    checkNear(Vec3(world[3]), camera.getPosition());
    checkNear(-Vec3(world[2]), camera.getForward());
    checkNear(Vec3(world[0]), camera.getRight());
    checkNear(Vec3(world[1]), camera.getUp());
    CHECK(glm::length(camera.getForward()) == Approx(1.0f).margin(TOLERANCE));
}

TEST_CASE("with no input the camera does not move", "[editor][edit]") {
    FlyCamera camera;
    const Vec3 start = camera.getPosition();
    const float startYaw = camera.getYaw();
    const float startPitch = camera.getPitch();

    run(camera, FlyCameraInput{}, 2.0f);
    run(camera, looking(), 2.0f);

    checkNear(camera.getPosition(), start);
    CHECK(camera.getYaw() == Approx(startYaw).margin(TOLERANCE));
    CHECK(camera.getPitch() == Approx(startPitch).margin(TOLERANCE));
}

TEST_CASE("mouse movement does nothing unless its button is held", "[editor][edit]") {
    FlyCamera camera;
    const Vec3 start = camera.getPosition();
    const float startYaw = camera.getYaw();
    const float startPitch = camera.getPitch();

    FlyCameraInput input;
    input.lookDelta = Vec2(500.0f, 500.0f);
    input.panDelta = Vec2(500.0f, 500.0f);
    input.viewHeight = VIEW_HEIGHT;
    input.fast = true;
    run(camera, input, 1.0f);

    checkNear(camera.getPosition(), start);
    CHECK(camera.getYaw() == Approx(startYaw).margin(TOLERANCE));
    CHECK(camera.getPitch() == Approx(startPitch).margin(TOLERANCE));
}

TEST_CASE("rotating turns the camera in place", "[editor][edit]") {
    FlyCamera camera;
    const Vec3 start = camera.getPosition();
    FlyCameraInput input = looking();
    input.lookDelta = Vec2(300.0f, -120.0f);
    input.fast = true;
    for (int i = 0; i < 30; i++) {
        camera.update(input, FRAME);
        checkNear(camera.getPosition(), start);
    }
    CHECK(camera.getYaw() != Approx(0.0f).margin(0.1f));
}

TEST_CASE("the mouse turns the camera the way it moves, smoothly", "[editor][edit]") {
    FlyCamera camera;
    const float startPitch = camera.getPitch();
    FlyCameraInput input = looking();
    input.lookDelta = Vec2(100.0f, 0.0f);
    camera.update(input, FRAME);

    const float turn = 100.0f * FlyCamera::LOOK_RADIANS_PER_PIXEL;
    CHECK(camera.getYaw() < 0.0f);
    CHECK(camera.getYaw() > -turn);
    run(camera, looking(), 1.0f);
    CHECK(camera.getYaw() == Approx(-turn).margin(TOLERANCE));
    CHECK(camera.getForward().x > 0.0f);

    input.lookDelta = Vec2(0.0f, 100.0f);
    camera.update(input, FRAME);
    run(camera, looking(), 1.0f);
    CHECK(camera.getPitch() == Approx(startPitch - turn).margin(TOLERANCE));
}

TEST_CASE("the pitch stops short of straight up and straight down", "[editor][edit]") {
    FlyCamera camera;
    const float limit = glm::radians(FlyCamera::MAX_PITCH_DEGREES);
    FlyCameraInput input = looking();

    input.lookDelta = Vec2(0.0f, -1.0e6f);
    camera.update(input, FRAME);
    run(camera, looking(), 1.0f);
    CHECK(camera.getPitch() == Approx(limit).margin(TOLERANCE));

    input.lookDelta = Vec2(0.0f, 1.0e6f);
    camera.update(input, FRAME);
    run(camera, looking(), 1.0f);
    CHECK(camera.getPitch() == Approx(-limit).margin(TOLERANCE));
    CHECK(std::isfinite(camera.getWorld()[0][0]));
}

TEST_CASE("a wheel step zooms smoothly along the front, and a step back returns", "[editor][edit]") {
    FlyCamera camera;
    const Vec3 start = camera.getPosition();
    const Vec3 forward = camera.getForward();
    const float step = FlyCamera::DEFAULT_REFERENCE_DISTANCE * FlyCamera::ZOOM_STEP_PER_METRE;

    camera.update(zooming(1.0f), FRAME);
    const float firstFrame = glm::dot(camera.getPosition() - start, forward);
    CHECK(firstFrame > 0.0f);
    CHECK(firstFrame < step * 0.5f);

    run(camera, FlyCameraInput{}, 1.0f);
    checkNear(camera.getPosition(), start + forward * step);

    camera.update(zooming(-1.0f), FRAME);
    run(camera, FlyCameraInput{}, 1.0f);
    checkNear(camera.getPosition(), start);
}

TEST_CASE("the reference distance is the distance to the pivot, within its limits", "[editor][edit]") {
    const FlyCamera camera;
    FlyCameraInput input;
    CHECK(camera.getReferenceDistance(input) == FlyCamera::DEFAULT_REFERENCE_DISTANCE);

    input.hasPivot = true;
    input.pivot = camera.getPosition() + Vec3(10.0f, 0.0f, 0.0f);
    CHECK(camera.getReferenceDistance(input) == Approx(10.0f));
    input.pivot = camera.getPosition() + Vec3(0.0f, -60.0f, 80.0f);
    CHECK(camera.getReferenceDistance(input) == Approx(100.0f));
    input.pivot = camera.getPosition();
    CHECK(camera.getReferenceDistance(input) == FlyCamera::MIN_REFERENCE_DISTANCE);
    CHECK(camera.getZoomStep(input) == Approx(FlyCamera::MIN_REFERENCE_DISTANCE * FlyCamera::ZOOM_STEP_PER_METRE));
    input.pivot = camera.getPosition() + Vec3(0.0f, 0.0f, 1.0e7f);
    CHECK(camera.getReferenceDistance(input) == FlyCamera::MAX_REFERENCE_DISTANCE);
    CHECK(camera.getZoomStep(input) == Approx(FlyCamera::MAX_REFERENCE_DISTANCE * FlyCamera::ZOOM_STEP_PER_METRE));
}

TEST_CASE("a zoom step follows the distance to the pivot", "[editor][edit]") {
    FlyCamera camera;
    const Vec3 start = camera.getPosition();
    const Vec3 forward = camera.getForward();
    FlyCameraInput input = zooming(1.0f);
    input.hasPivot = true;
    input.pivot = start + forward * 50.0f;

    camera.update(input, FRAME);
    run(camera, FlyCameraInput{}, 1.0f);
    checkNear(camera.getPosition(), start + forward * (50.0f * FlyCamera::ZOOM_STEP_PER_METRE), 0.001f);
}

static FlyCameraInput withSpeedKeys(FlyCameraInput input, const bool fast, const bool slow) {
    input.fast = fast;
    input.slow = slow;
    return input;
}

static float yawAfterTurn(const bool fast, const bool slow) {
    FlyCamera camera;
    FlyCameraInput input = withSpeedKeys(looking(), fast, slow);
    input.lookDelta = Vec2(100.0f, 0.0f);
    camera.update(input, FRAME);
    run(camera, looking(), 1.0f);
    return camera.getYaw();
}

TEST_CASE("shift speeds up and ctrl slows down the rotation, the pan and the zoom", "[editor][edit]") {
    const float fast = FlyCamera::FAST_MULTIPLIER;
    const float slow = FlyCamera::SLOW_MULTIPLIER;
    CHECK(FlyCamera::getSpeedMultiplier(FlyCameraInput{}) == 1.0f);
    CHECK(FlyCamera::getSpeedMultiplier(withSpeedKeys(FlyCameraInput{}, true, false)) == Approx(fast));
    CHECK(FlyCamera::getSpeedMultiplier(withSpeedKeys(FlyCameraInput{}, false, true)) == Approx(slow));
    CHECK(FlyCamera::getSpeedMultiplier(withSpeedKeys(FlyCameraInput{}, true, true)) == Approx(fast * slow));

    const float normalYaw = yawAfterTurn(false, false);
    CHECK(yawAfterTurn(true, false) == Approx(normalYaw * fast).margin(TOLERANCE));
    CHECK(yawAfterTurn(false, true) == Approx(normalYaw * slow).margin(TOLERANCE));

    FlyCamera camera;
    FlyCameraInput pan;
    pan.panning = true;
    pan.viewHeight = VIEW_HEIGHT;
    const float normalPan = camera.getPanMetresPerPixel(pan);
    CHECK(camera.getPanMetresPerPixel(withSpeedKeys(pan, true, false)) == Approx(normalPan * fast));
    CHECK(camera.getPanMetresPerPixel(withSpeedKeys(pan, false, true)) == Approx(normalPan * slow));

    const float normalStep = camera.getZoomStep(zooming(1.0f));
    CHECK(camera.getZoomStep(withSpeedKeys(zooming(1.0f), true, false)) == Approx(normalStep * fast));
    CHECK(camera.getZoomStep(withSpeedKeys(zooming(1.0f), false, true)) == Approx(normalStep * slow));

    const Vec3 start = camera.getPosition();
    const Vec3 forward = camera.getForward();
    camera.update(withSpeedKeys(zooming(1.0f), true, false), FRAME);
    run(camera, FlyCameraInput{}, 1.0f);
    checkNear(camera.getPosition(), start + forward * (normalStep * fast));
}

TEST_CASE("quick zoom steps toward the pivot shrink instead of passing it", "[editor][edit]") {
    FlyCamera camera;
    const Vec3 forward = camera.getForward();
    FlyCameraInput input = zooming(1.0f);
    input.hasPivot = true;
    input.pivot = camera.getPosition() + forward * 10.0f;

    const int steps = 8;
    for (int i = 0; i < steps; i++) {
        camera.update(input, FRAME);
    }
    run(camera, FlyCameraInput{}, 1.0f);

    const float expected = 10.0f * std::pow(1.0f - FlyCamera::ZOOM_STEP_PER_METRE, static_cast<float>(steps));
    CHECK(glm::dot(input.pivot - camera.getPosition(), forward) == Approx(expected).margin(0.001f));
}

TEST_CASE("a zoom and a pan cover the same ground after the same time at any frame rate", "[editor][edit]") {
    FlyCameraInput pan;
    pan.panning = true;
    pan.viewHeight = VIEW_HEIGHT;
    pan.panDelta = Vec2(80.0f, 30.0f);

    FlyCamera slow;
    FlyCamera fast;
    slow.update(zooming(3.0f), 0.0f);
    slow.update(pan, 0.0f);
    fast.update(zooming(3.0f), 0.0f);
    fast.update(pan, 0.0f);
    const Vec3 start = slow.getPosition();
    run(slow, FlyCameraInput{}, 0.2f, 1.0f / 30.0f);
    run(fast, FlyCameraInput{}, 0.2f, 1.0f / 240.0f);
    CHECK(glm::length(slow.getPosition() - start) > 0.5f);
    checkNear(slow.getPosition(), fast.getPosition());
}

TEST_CASE("a long frame zooms no further than the longest step allows, and a negative one not at all", "[editor][edit]") {
    FlyCamera camera;
    const Vec3 start = camera.getPosition();
    const float step = camera.getZoomStep(zooming(1.0f));

    camera.update(zooming(1.0f), 5.0f);
    const float consumed = 1.0f - std::exp(-FlyCamera::ZOOM_SHARPNESS * FlyCamera::MAX_DELTA_TIME);
    CHECK(glm::length(camera.getPosition() - start) == Approx(step * consumed).margin(TOLERANCE));

    FlyCamera rewound;
    rewound.update(zooming(1.0f), -1.0f);
    checkNear(rewound.getPosition(), start);
}

TEST_CASE("a pan eases the camera across the view until the scene has followed the cursor", "[editor][edit]") {
    FlyCamera camera;
    const Vec3 start = camera.getPosition();
    const Vec3 right = camera.getRight();
    const Vec3 up = camera.getUp();
    const float startYaw = camera.getYaw();

    FlyCameraInput input;
    input.panning = true;
    input.viewHeight = VIEW_HEIGHT;
    input.panDelta = Vec2(100.0f, -40.0f);
    const float metresPerPixel = 2.0f * FlyCamera::DEFAULT_REFERENCE_DISTANCE * std::tan(glm::radians(TechEngine::Camera{}.verticalFieldOfView) * 0.5f) / VIEW_HEIGHT;
    CHECK(camera.getPanMetresPerPixel(input) == Approx(metresPerPixel));

    const Vec3 full = -right * (100.0f * metresPerPixel) - up * (40.0f * metresPerPixel);
    camera.update(input, FRAME);
    const float firstFrame = glm::dot(camera.getPosition() - start, glm::normalize(full));
    CHECK(firstFrame > 0.0f);
    CHECK(firstFrame < glm::length(full) * 0.5f);
    CHECK(camera.isMoving());

    FlyCameraInput held;
    held.panning = true;
    held.viewHeight = VIEW_HEIGHT;
    run(camera, held, 1.0f);
    checkNear(camera.getPosition(), start + full);
    CHECK(camera.getYaw() == startYaw);
    CHECK_FALSE(camera.isMoving());
}

TEST_CASE("a pan follows the distance to the pivot, and needs the view's height", "[editor][edit]") {
    FlyCamera camera;
    FlyCameraInput input;
    input.panning = true;
    input.viewHeight = VIEW_HEIGHT;
    input.panDelta = Vec2(10.0f, 0.0f);
    const float withoutPivot = camera.getPanMetresPerPixel(input);

    input.hasPivot = true;
    input.pivot = camera.getPosition() + Vec3(0.0f, 0.0f, -50.0f);
    CHECK(camera.getPanMetresPerPixel(input) == Approx(withoutPivot * 50.0f / FlyCamera::DEFAULT_REFERENCE_DISTANCE));

    input.viewHeight = 0.0f;
    CHECK(camera.getPanMetresPerPixel(input) == 0.0f);
    const Vec3 start = camera.getPosition();
    camera.update(input, FRAME);
    checkNear(camera.getPosition(), start);
}

TEST_CASE("focus flies to look at the target from a distance that frames it", "[editor][edit]") {
    FlyCamera camera;
    const Vec3 start = camera.getPosition();
    const Vec3 target(10.0f, 0.0f, -20.0f);
    const float radius = 2.0f;
    const float distance = radius * FlyCamera::FOCUS_DISTANCE_PER_RADIUS;
    camera.focus(target, radius);
    CHECK(camera.isFocusing());

    run(camera, FlyCameraInput{}, FlyCamera::FOCUS_DURATION * 0.5f);
    CHECK(camera.isFocusing());
    const float midway = glm::length(camera.getPosition() - target);
    CHECK(midway < glm::length(start - target));
    CHECK(midway > distance);

    run(camera, FlyCameraInput{}, FlyCamera::FOCUS_DURATION);
    CHECK_FALSE(camera.isFocusing());
    CHECK(glm::length(camera.getPosition() - target) == Approx(distance).margin(0.001f));
    CHECK(glm::dot(camera.getForward(), glm::normalize(target - camera.getPosition())) == Approx(1.0f).margin(TOLERANCE));

    const Vec3 arrived = camera.getPosition();
    run(camera, FlyCameraInput{}, 1.0f);
    checkNear(camera.getPosition(), arrived);
}

TEST_CASE("focus keeps a minimum distance from a target with no size", "[editor][edit]") {
    FlyCamera camera;
    const Vec3 target(0.0f, 0.0f, -3.0f);
    camera.focus(target, 0.0f);
    run(camera, FlyCameraInput{}, FlyCamera::FOCUS_DURATION + FRAME);
    CHECK(glm::length(camera.getPosition() - target) == Approx(FlyCamera::MIN_FOCUS_DISTANCE).margin(0.001f));
}

TEST_CASE("focus on the camera's own position keeps its heading and backs away", "[editor][edit]") {
    FlyCamera camera;
    const Vec3 target = camera.getPosition();
    const Vec3 forward = camera.getForward();
    camera.focus(target, 1.0f);
    run(camera, FlyCameraInput{}, FlyCamera::FOCUS_DURATION + FRAME);
    checkNear(camera.getForward(), forward);
    checkNear(camera.getPosition(), target - forward * FlyCamera::FOCUS_DISTANCE_PER_RADIUS, 0.001f);
}

TEST_CASE("focus on a target straight below still ends with the target in the middle of the view", "[editor][edit]") {
    FlyCamera camera;
    const Vec3 target = camera.getPosition() - Vec3(0.0f, 10.0f, 0.0f);
    camera.focus(target, 1.0f);
    run(camera, FlyCameraInput{}, FlyCamera::FOCUS_DURATION + FRAME);
    CHECK(camera.getPitch() == Approx(-glm::radians(FlyCamera::MAX_PITCH_DEGREES)).margin(TOLERANCE));
    CHECK(glm::dot(camera.getForward(), glm::normalize(target - camera.getPosition())) == Approx(1.0f).margin(TOLERANCE));
}

TEST_CASE("focus turns the short way round", "[editor][edit]") {
    FlyCamera camera;
    turnTo(camera, 170.0f);
    REQUIRE(camera.getYaw() == Approx(glm::radians(170.0f)).margin(TOLERANCE));

    const float targetYaw = glm::radians(-170.0f);
    const Vec3 direction(-std::sin(targetYaw), 0.0f, -std::cos(targetYaw));
    camera.focus(camera.getPosition() + direction * 10.0f, 1.0f);
    while (camera.isFocusing()) {
        camera.update(FlyCameraInput{}, FRAME);
        CHECK(camera.getYaw() >= glm::radians(170.0f) - TOLERANCE);
        CHECK(camera.getYaw() <= glm::radians(190.0f) + TOLERANCE);
    }
    CHECK(glm::dot(camera.getForward(), direction) == Approx(1.0f).margin(TOLERANCE));
}

TEST_CASE("rotating, panning or zooming cancels a focus, and holding a button still does not", "[editor][edit]") {
    const Vec3 target(0.0f, 0.0f, -30.0f);

    FlyCamera held;
    held.focus(target, 1.0f);
    FlyCameraInput both = looking();
    both.panning = true;
    both.viewHeight = VIEW_HEIGHT;
    run(held, both, FlyCamera::FOCUS_DURATION + FRAME);
    CHECK_FALSE(held.isFocusing());
    CHECK(glm::length(held.getPosition() - target) == Approx(FlyCamera::FOCUS_DISTANCE_PER_RADIUS).margin(0.001f));

    FlyCamera turned;
    turned.focus(target, 1.0f);
    FlyCameraInput look = looking();
    look.lookDelta = Vec2(1.0f, 0.0f);
    turned.update(look, FRAME);
    CHECK_FALSE(turned.isFocusing());

    FlyCamera panned;
    panned.focus(target, 1.0f);
    FlyCameraInput pan;
    pan.panning = true;
    pan.viewHeight = VIEW_HEIGHT;
    pan.panDelta = Vec2(1.0f, 0.0f);
    panned.update(pan, FRAME);
    CHECK_FALSE(panned.isFocusing());

    FlyCamera zoomed;
    zoomed.focus(target, 1.0f);
    zoomed.update(zooming(1.0f), FRAME);
    CHECK_FALSE(zoomed.isFocusing());
    run(zoomed, FlyCameraInput{}, 1.0f);
    CHECK(glm::length(zoomed.getPosition() - target) > FlyCamera::FOCUS_DISTANCE_PER_RADIUS + 1.0f);
}

TEST_CASE("a focus drops a zoom and a pan still under way", "[editor][edit]") {
    FlyCamera camera;
    camera.update(zooming(5.0f), 0.0f);
    FlyCameraInput pan;
    pan.panning = true;
    pan.viewHeight = VIEW_HEIGHT;
    pan.panDelta = Vec2(200.0f, 0.0f);
    camera.update(pan, 0.0f);

    const Vec3 target(5.0f, 0.0f, 0.0f);
    camera.focus(target, 1.0f);
    run(camera, FlyCameraInput{}, FlyCamera::FOCUS_DURATION + 1.0f);
    CHECK(glm::length(camera.getPosition() - target) == Approx(FlyCamera::FOCUS_DISTANCE_PER_RADIUS).margin(0.001f));
}

TEST_CASE("the camera asks for frames only while something is still easing", "[editor][edit]") {
    FlyCamera camera;
    CHECK_FALSE(camera.isMoving());

    FlyCameraInput turn = looking();
    turn.lookDelta = Vec2(40.0f, 0.0f);
    camera.update(turn, FRAME);
    CHECK(camera.isMoving());
    run(camera, looking(), 1.0f);
    CHECK_FALSE(camera.isMoving());

    camera.update(zooming(1.0f), FRAME);
    CHECK(camera.isMoving());
    run(camera, FlyCameraInput{}, 2.0f);
    CHECK_FALSE(camera.isMoving());

    camera.focus(Vec3(3.0f, 0.0f, -4.0f), 1.0f);
    CHECK(camera.isMoving());
    run(camera, FlyCameraInput{}, FlyCamera::FOCUS_DURATION + FRAME);
    CHECK_FALSE(camera.isMoving());

    FlyCameraInput pan;
    pan.panning = true;
    pan.viewHeight = VIEW_HEIGHT;
    pan.panDelta = Vec2(25.0f, 10.0f);
    camera.update(pan, FRAME);
    CHECK(camera.isMoving());
    run(camera, FlyCameraInput{}, 1.0f);
    CHECK_FALSE(camera.isMoving());
}
