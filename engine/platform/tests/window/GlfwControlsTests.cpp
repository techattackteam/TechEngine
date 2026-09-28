#include <window/GlfwControls.hpp>

#include <catch2/catch_test_macros.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstddef>
#include <limits>

using TechEngine::Key;
using TechEngine::MouseButton;
using TechEngine::translateGlfwKey;
using TechEngine::translateGlfwMouseButton;

TEST_CASE("every named GLFW key translates to its own engine key, densely and in GLFW order", "[platform][input][glfw]") {
    std::size_t next = 0;
    for (int glfwKey = GLFW_KEY_UNKNOWN; glfwKey <= GLFW_KEY_LAST + 1; glfwKey++) {
        const Key key = translateGlfwKey(glfwKey);
        if (key != Key::Unknown) {
            INFO("GLFW key " << glfwKey);
            CHECK(static_cast<std::size_t>(key) == next);
            next++;
        }
    }
    CHECK(next == TechEngine::KEY_COUNT);
}

TEST_CASE("each run of GLFW key values starts and ends on the engine key of the same name", "[platform][input][glfw]") {
    CHECK(translateGlfwKey(GLFW_KEY_SPACE) == Key::Space);
    CHECK(translateGlfwKey(GLFW_KEY_APOSTROPHE) == Key::Apostrophe);
    CHECK(translateGlfwKey(GLFW_KEY_0) == Key::Number0);
    CHECK(translateGlfwKey(GLFW_KEY_9) == Key::Number9);
    CHECK(translateGlfwKey(GLFW_KEY_A) == Key::A);
    CHECK(translateGlfwKey(GLFW_KEY_W) == Key::W);
    CHECK(translateGlfwKey(GLFW_KEY_Z) == Key::Z);
    CHECK(translateGlfwKey(GLFW_KEY_GRAVE_ACCENT) == Key::GraveAccent);
    CHECK(translateGlfwKey(GLFW_KEY_WORLD_1) == Key::World1);
    CHECK(translateGlfwKey(GLFW_KEY_WORLD_2) == Key::World2);
    CHECK(translateGlfwKey(GLFW_KEY_ESCAPE) == Key::Escape);
    CHECK(translateGlfwKey(GLFW_KEY_RIGHT) == Key::ArrowRight);
    CHECK(translateGlfwKey(GLFW_KEY_UP) == Key::ArrowUp);
    CHECK(translateGlfwKey(GLFW_KEY_END) == Key::End);
    CHECK(translateGlfwKey(GLFW_KEY_CAPS_LOCK) == Key::CapsLock);
    CHECK(translateGlfwKey(GLFW_KEY_PAUSE) == Key::Pause);
    CHECK(translateGlfwKey(GLFW_KEY_F1) == Key::F1);
    CHECK(translateGlfwKey(GLFW_KEY_F25) == Key::F25);
    CHECK(translateGlfwKey(GLFW_KEY_KP_0) == Key::Keypad0);
    CHECK(translateGlfwKey(GLFW_KEY_KP_EQUAL) == Key::KeypadEqual);
    CHECK(translateGlfwKey(GLFW_KEY_LEFT_SHIFT) == Key::LeftShift);
    CHECK(translateGlfwKey(GLFW_KEY_RIGHT_SUPER) == Key::RightSuper);
    CHECK(translateGlfwKey(GLFW_KEY_MENU) == Key::Menu);
}

TEST_CASE("unnamed and out-of-range GLFW key values translate to Unknown", "[platform][input][glfw]") {
    CHECK(translateGlfwKey(GLFW_KEY_UNKNOWN) == Key::Unknown);
    CHECK(translateGlfwKey(std::numeric_limits<int>::min()) == Key::Unknown);
    CHECK(translateGlfwKey(0) == Key::Unknown);
    CHECK(translateGlfwKey(GLFW_KEY_SPACE + 1) == Key::Unknown);
    CHECK(translateGlfwKey(GLFW_KEY_WORLD_2 + 1) == Key::Unknown);
    CHECK(translateGlfwKey(GLFW_KEY_ESCAPE - 1) == Key::Unknown);
    CHECK(translateGlfwKey(GLFW_KEY_SPACE + 256) == Key::Unknown);
    CHECK(translateGlfwKey(GLFW_KEY_LAST + 1) == Key::Unknown);
    CHECK(translateGlfwKey(std::numeric_limits<int>::max()) == Key::Unknown);
}

TEST_CASE("every GLFW mouse button translates to its own engine button, in GLFW order", "[platform][input][glfw]") {
    CHECK(translateGlfwMouseButton(GLFW_MOUSE_BUTTON_LEFT) == MouseButton::Left);
    CHECK(translateGlfwMouseButton(GLFW_MOUSE_BUTTON_RIGHT) == MouseButton::Right);
    CHECK(translateGlfwMouseButton(GLFW_MOUSE_BUTTON_MIDDLE) == MouseButton::Middle);
    CHECK(translateGlfwMouseButton(GLFW_MOUSE_BUTTON_4) == MouseButton::Extra1);
    CHECK(translateGlfwMouseButton(GLFW_MOUSE_BUTTON_LAST) == MouseButton::Extra5);

    std::size_t next = 0;
    for (int glfwButton = 0; glfwButton <= GLFW_MOUSE_BUTTON_LAST; glfwButton++) {
        const MouseButton button = translateGlfwMouseButton(glfwButton);
        INFO("GLFW mouse button " << glfwButton);
        REQUIRE(button != MouseButton::Unknown);
        CHECK(static_cast<std::size_t>(button) == next);
        next++;
    }
    CHECK(next == TechEngine::MOUSE_BUTTON_COUNT);
}

TEST_CASE("out-of-range GLFW mouse buttons translate to Unknown", "[platform][input][glfw]") {
    CHECK(translateGlfwMouseButton(-1) == MouseButton::Unknown);
    CHECK(translateGlfwMouseButton(std::numeric_limits<int>::min()) == MouseButton::Unknown);
    CHECK(translateGlfwMouseButton(GLFW_MOUSE_BUTTON_LAST + 1) == MouseButton::Unknown);
    CHECK(translateGlfwMouseButton(GLFW_MOUSE_BUTTON_LAST + 256) == MouseButton::Unknown);
    CHECK(translateGlfwMouseButton(std::numeric_limits<int>::max()) == MouseButton::Unknown);
}
