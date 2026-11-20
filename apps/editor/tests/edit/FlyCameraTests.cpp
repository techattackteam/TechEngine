#include <TechEngine/base/math/Math.hpp>

#include <edit/FlyCamera.hpp>

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

static FlyCameraInput looking() {
    FlyCameraInput input;
    input.looking = true;
    return input;
}

static FlyCameraInput movingForward() {
    FlyCameraInput input = looking();
    input.move = Vec3(0.0f, 0.0f, 1.0f);
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
    run(camera, movingForward(), 0.5f);

    const Mat4 world = camera.getWorld();
    checkNear(Vec3(world[3]), camera.getPosition());
    checkNear(-Vec3(world[2]), camera.getForward());
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
    CHECK(camera.getVelocity() == Vec3(0.0f));
}

TEST_CASE("keys, the mouse and the wheel do nothing unless the right mouse button is held", "[editor][edit]") {
    FlyCamera camera;
    const Vec3 start = camera.getPosition();
    const float startYaw = camera.getYaw();

    FlyCameraInput input;
    input.move = Vec3(1.0f, 1.0f, 1.0f);
    input.lookDelta = Vec2(500.0f, 500.0f);
    input.wheel = 5.0f;
    input.fast = true;
    run(camera, input, 1.0f);

    checkNear(camera.getPosition(), start);
    CHECK(camera.getYaw() == Approx(startYaw).margin(TOLERANCE));
    CHECK(camera.getSpeedScale() == 1.0f);
}

TEST_CASE("moving forward ramps the speed up smoothly along the camera's front", "[editor][edit]") {
    FlyCamera camera;
    const FlyCameraInput input = movingForward();
    const float targetSpeed = camera.getTargetSpeed(input);
    REQUIRE(targetSpeed == Approx(FlyCamera::DEFAULT_SPEED_DISTANCE * FlyCamera::SPEED_PER_METRE));

    camera.update(input, FRAME);
    float previousSpeed = glm::length(camera.getVelocity());
    CHECK(previousSpeed > 0.0f);
    CHECK(previousSpeed < targetSpeed * 0.25f);
    for (int i = 0; i < 30; i++) {
        camera.update(input, FRAME);
        const float speed = glm::length(camera.getVelocity());
        CHECK(speed > previousSpeed);
        CHECK(speed <= targetSpeed);
        previousSpeed = speed;
    }

    run(camera, input, 2.0f);
    CHECK(glm::length(camera.getVelocity()) == Approx(targetSpeed).epsilon(0.01));
    CHECK(glm::dot(glm::normalize(camera.getVelocity()), camera.getForward()) == Approx(1.0f).margin(TOLERANCE));
}

TEST_CASE("releasing the keys glides to a full stop without moving back", "[editor][edit]") {
    FlyCamera camera;
    const FlyCameraInput input = movingForward();
    run(camera, input, 2.0f);
    const Vec3 forward = camera.getForward();
    const Vec3 released = camera.getPosition();

    float previousTravel = 0.0f;
    for (int i = 0; i < 180; i++) {
        camera.update(FlyCameraInput{}, FRAME);
        const float travel = glm::dot(camera.getPosition() - released, forward);
        CHECK(travel >= previousTravel);
        previousTravel = travel;
    }

    CHECK(camera.getVelocity() == Vec3(0.0f));
    const float glide = camera.getTargetSpeed(input) / FlyCamera::MOVE_SHARPNESS;
    CHECK(previousTravel > glide * 0.8f);
    CHECK(previousTravel < glide * 1.1f);
}

TEST_CASE("the speed is proportional to the distance to the pivot, within its limits", "[editor][edit]") {
    const FlyCamera camera;
    FlyCameraInput input = movingForward();
    input.hasPivot = true;

    input.pivot = camera.getPosition() + Vec3(10.0f, 0.0f, 0.0f);
    CHECK(camera.getTargetSpeed(input) == Approx(10.0f * FlyCamera::SPEED_PER_METRE));
    input.pivot = camera.getPosition() + Vec3(0.0f, -60.0f, 80.0f);
    CHECK(camera.getTargetSpeed(input) == Approx(100.0f * FlyCamera::SPEED_PER_METRE));

    input.pivot = camera.getPosition();
    CHECK(camera.getTargetSpeed(input) == Approx(FlyCamera::MIN_SPEED_DISTANCE * FlyCamera::SPEED_PER_METRE));
    input.pivot = camera.getPosition() + Vec3(0.0f, 0.0f, 1.0e7f);
    CHECK(camera.getTargetSpeed(input) == Approx(FlyCamera::MAX_SPEED_DISTANCE * FlyCamera::SPEED_PER_METRE));

    input.pivot = camera.getPosition() + Vec3(10.0f, 0.0f, 0.0f);
    input.fast = true;
    CHECK(camera.getTargetSpeed(input) == Approx(10.0f * FlyCamera::SPEED_PER_METRE * FlyCamera::FAST_MULTIPLIER));

    input.hasPivot = false;
    input.fast = false;
    CHECK(camera.getTargetSpeed(input) == Approx(FlyCamera::DEFAULT_SPEED_DISTANCE * FlyCamera::SPEED_PER_METRE));
}

TEST_CASE("flying at the pivot slows down as it gets close and does not overshoot", "[editor][edit]") {
    FlyCamera camera;
    FlyCameraInput input = movingForward();
    input.hasPivot = true;
    input.pivot = camera.getPosition() + camera.getForward() * 50.0f;

    run(camera, input, 1.0f);
    float previousSpeed = glm::length(camera.getVelocity());
    for (int i = 0; i < 120; i++) {
        camera.update(input, FRAME);
        const float speed = glm::length(camera.getVelocity());
        CHECK(speed < previousSpeed);
        CHECK(glm::dot(input.pivot - camera.getPosition(), camera.getForward()) > 0.0f);
        previousSpeed = speed;
    }
}

TEST_CASE("the velocity after the same time does not depend on the frame rate", "[editor][edit]") {
    FlyCamera slow;
    FlyCamera fast;
    run(slow, movingForward(), 0.5f, 1.0f / 30.0f);
    run(fast, movingForward(), 0.5f, 1.0f / 240.0f);

    checkNear(slow.getVelocity(), fast.getVelocity(), 0.001f);
    const float slowTravel = glm::length(slow.getPosition() - FlyCamera().getPosition());
    const float fastTravel = glm::length(fast.getPosition() - FlyCamera().getPosition());
    CHECK(slowTravel == Approx(fastTravel).epsilon(0.05));
}

TEST_CASE("a long frame moves the camera no further than the longest step allows", "[editor][edit]") {
    FlyCamera camera;
    const Vec3 start = camera.getPosition();
    const FlyCameraInput input = movingForward();

    camera.update(input, 5.0f);
    const float clampedSpeed = camera.getTargetSpeed(input) * (1.0f - std::exp(-FlyCamera::MOVE_SHARPNESS * FlyCamera::MAX_DELTA_TIME));
    CHECK(glm::length(camera.getPosition() - start) == Approx(clampedSpeed * FlyCamera::MAX_DELTA_TIME).margin(TOLERANCE));

    FlyCamera rewound;
    rewound.update(input, -1.0f);
    checkNear(rewound.getPosition(), start);
}

TEST_CASE("diagonal movement is no faster than straight movement", "[editor][edit]") {
    FlyCamera camera;
    FlyCameraInput input = looking();
    input.move = Vec3(1.0f, 1.0f, 1.0f);
    run(camera, input, 3.0f);
    CHECK(glm::length(camera.getVelocity()) == Approx(camera.getTargetSpeed(input)).epsilon(0.01));
}

TEST_CASE("Q and E move along the world's up, whatever the camera's pitch", "[editor][edit]") {
    FlyCamera camera;
    FlyCameraInput input = looking();
    input.move = Vec3(0.0f, 1.0f, 0.0f);
    run(camera, input, 2.0f);
    const Vec3 direction = glm::normalize(camera.getVelocity());
    checkNear(direction, Vec3(0.0f, 1.0f, 0.0f));
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

TEST_CASE("the wheel scales the speed while looking, within its limits", "[editor][edit]") {
    FlyCamera camera;
    FlyCameraInput input = looking();
    input.wheel = 1.0f;
    camera.update(input, FRAME);
    CHECK(camera.getSpeedScale() == Approx(FlyCamera::SPEED_STEP));
    CHECK(camera.getTargetSpeed(looking()) == Approx(FlyCamera::DEFAULT_SPEED_DISTANCE * FlyCamera::SPEED_PER_METRE * FlyCamera::SPEED_STEP));

    input.wheel = -1000.0f;
    camera.update(input, FRAME);
    CHECK(camera.getSpeedScale() == Approx(FlyCamera::MIN_SPEED_SCALE));

    input.wheel = 1000.0f;
    camera.update(input, FRAME);
    CHECK(camera.getSpeedScale() == Approx(FlyCamera::MAX_SPEED_SCALE));
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

TEST_CASE("looking or moving cancels a focus, and holding the button still does not", "[editor][edit]") {
    const Vec3 target(0.0f, 0.0f, -30.0f);

    FlyCamera held;
    held.focus(target, 1.0f);
    run(held, looking(), FlyCamera::FOCUS_DURATION + FRAME);
    CHECK_FALSE(held.isFocusing());
    CHECK(glm::length(held.getPosition() - target) == Approx(FlyCamera::FOCUS_DISTANCE_PER_RADIUS).margin(0.001f));

    FlyCamera moved;
    moved.focus(target, 1.0f);
    moved.update(FlyCameraInput{}, FRAME);
    moved.update(movingForward(), FRAME);
    CHECK_FALSE(moved.isFocusing());
    run(moved, FlyCameraInput{}, FlyCamera::FOCUS_DURATION + 1.0f);
    CHECK(glm::length(moved.getPosition() - target) > FlyCamera::FOCUS_DISTANCE_PER_RADIUS + 1.0f);

    FlyCamera turned;
    turned.focus(target, 1.0f);
    FlyCameraInput look = looking();
    look.lookDelta = Vec2(1.0f, 0.0f);
    turned.update(look, FRAME);
    CHECK_FALSE(turned.isFocusing());
}

TEST_CASE("a focus stops the camera's glide", "[editor][edit]") {
    FlyCamera camera;
    run(camera, movingForward(), 2.0f);
    REQUIRE(glm::length(camera.getVelocity()) > 1.0f);

    const Vec3 target(5.0f, 0.0f, 0.0f);
    camera.focus(target, 1.0f);
    CHECK(camera.getVelocity() == Vec3(0.0f));
    run(camera, FlyCameraInput{}, FlyCamera::FOCUS_DURATION + 1.0f);
    CHECK(glm::length(camera.getPosition() - target) == Approx(FlyCamera::FOCUS_DISTANCE_PER_RADIUS).margin(0.001f));
}
