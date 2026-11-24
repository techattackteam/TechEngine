#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <edit/snapshot/EditorSnapshot.hpp>
#include <ui/PanelAppProbe.hpp>
#include <ui/panels/SceneViewPanel.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/geometric.hpp>
#include <imgui.h>

#include <functional>

using Catch::Approx;
using TechEngine::ComponentRow;
using TechEngine::EditorSnapshot;
using TechEngine::Entity;
using TechEngine::SceneViewPanel;
using TechEngine::Transform;
using TechEngine::Vec3;
using TechEngine::ViewRequest;
using TechEngineTests::PanelAppProbe;

static constexpr float FRAME = 1.0f / 60.0f;
static constexpr ImVec2 PANEL_POSITION(0.0f, 0.0f);
static constexpr ImVec2 PANEL_SIZE(640.0f, 480.0f);
static constexpr ImVec2 IN_VIEW(320.0f, 240.0f);
static constexpr ImVec2 OUTSIDE_VIEW(900.0f, 700.0f);

struct HeadlessImGui {
    ImGuiContext* previous = nullptr;
    ImGuiContext* context = nullptr;
    PanelAppProbe app;
    SceneViewPanel panel{app.commands};
    EditorSnapshot snapshot;
    bool otherWindowFocused = false;

    HeadlessImGui() {
        previous = ImGui::GetCurrentContext();
        context = ImGui::CreateContext();
        // CreateContext leaves an earlier context current.
        ImGui::SetCurrentContext(context);
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.DisplaySize = ImVec2(1024.0f, 768.0f);
        io.DeltaTime = FRAME;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
        frame([](ImGuiIO& events) {
            events.AddMousePosEvent(OUTSIDE_VIEW.x, OUTSIDE_VIEW.y);
        });
        frame();
        frame();
    }

    ~HeadlessImGui() {
        ImGui::DestroyContext(context);
        ImGui::SetCurrentContext(previous);
    }

    HeadlessImGui(const HeadlessImGui&) = delete;
    HeadlessImGui& operator=(const HeadlessImGui&) = delete;

    void frame(const std::function<void(ImGuiIO&)>& events = {}) {
        ImGuiIO& io = ImGui::GetIO();
        if (events) {
            events(io);
        }
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(PANEL_POSITION);
        ImGui::SetNextWindowSize(PANEL_SIZE);
        panel.draw(snapshot);
        if (otherWindowFocused) {
            ImGui::SetNextWindowFocus();
            ImGui::SetNextWindowPos(ImVec2(700.0f, 0.0f));
            ImGui::SetNextWindowSize(ImVec2(200.0f, 200.0f));
            ImGui::Begin("Hierarchy");
            ImGui::End();
        }
        ImGui::Render();
    }

    void settle(const int frames = 120) {
        for (int i = 0; i < frames; i++) {
            frame();
        }
    }

    ViewRequest describe() const {
        return panel.describe(true);
    }
};

static Vec3 positionOf(const ViewRequest& request) {
    return Vec3(request.pose.world[3]);
}

static Vec3 forwardOf(const ViewRequest& request) {
    return -Vec3(request.pose.world[2]);
}

static void selectEntityAt(EditorSnapshot& snapshot, const Vec3& position) {
    snapshot.active = Entity{0, 0};
    snapshot.selection = {snapshot.active};
    ComponentRow transform;
    transform.type = TechEngine::componentTypeId<Transform>();
    snapshot.components = {transform};
    snapshot.world.position = position;
    snapshot.world.scale = Vec3(1.0f);
}

TEST_CASE("dragging with the right mouse button turns the scene view's camera in place", "[editor][ui]") {
    HeadlessImGui imgui;
    const ViewRequest before = imgui.describe();

    imgui.frame([](ImGuiIO& io) {
        io.AddMousePosEvent(IN_VIEW.x, IN_VIEW.y);
    });
    imgui.frame([](ImGuiIO& io) {
        io.AddMouseButtonEvent(ImGuiMouseButton_Right, true);
    });
    CHECK(imgui.panel.wantsCursorLocked());
    for (int i = 1; i <= 20; i++) {
        imgui.frame([i](ImGuiIO& io) {
            io.AddMousePosEvent(IN_VIEW.x + 10.0f * static_cast<float>(i), IN_VIEW.y);
        });
        CHECK(imgui.panel.wantsCursorLocked());
    }
    imgui.frame([](ImGuiIO& io) {
        io.AddMouseButtonEvent(ImGuiMouseButton_Right, false);
    });
    CHECK_FALSE(imgui.panel.wantsCursorLocked());
    imgui.settle();

    const ViewRequest after = imgui.describe();
    CHECK(forwardOf(after).x > forwardOf(before).x + 0.1f);
    CHECK(glm::length(positionOf(after) - positionOf(before)) == Approx(0.0f).margin(0.0001f));
}

TEST_CASE("the right mouse button does nothing when it is pressed outside the scene view", "[editor][ui]") {
    HeadlessImGui imgui;
    const ViewRequest before = imgui.describe();

    imgui.frame([](ImGuiIO& io) {
        io.AddMouseButtonEvent(ImGuiMouseButton_Right, true);
    });
    for (int i = 1; i <= 10; i++) {
        imgui.frame([i](ImGuiIO& io) {
            io.AddMousePosEvent(IN_VIEW.x + 10.0f * static_cast<float>(i), IN_VIEW.y);
        });
    }
    CHECK_FALSE(imgui.panel.wantsCursorLocked());
    imgui.settle();
    CHECK(forwardOf(imgui.describe()).x == Approx(forwardOf(before).x).margin(0.0001f));
}

TEST_CASE("dragging with the middle mouse button pans the scene view's camera without turning it, cursor hidden", "[editor][ui]") {
    HeadlessImGui imgui;
    const ViewRequest before = imgui.describe();

    imgui.frame([](ImGuiIO& io) {
        io.AddMousePosEvent(IN_VIEW.x, IN_VIEW.y);
    });
    CHECK_FALSE(imgui.panel.wantsCursorLocked());
    imgui.frame([](ImGuiIO& io) {
        io.AddMouseButtonEvent(ImGuiMouseButton_Middle, true);
    });
    CHECK(imgui.panel.wantsCursorLocked());
    for (int i = 1; i <= 10; i++) {
        imgui.frame([i](ImGuiIO& io) {
            io.AddMousePosEvent(IN_VIEW.x + 10.0f * static_cast<float>(i), IN_VIEW.y);
        });
        CHECK(imgui.panel.wantsCursorLocked());
    }
    imgui.frame([](ImGuiIO& io) {
        io.AddMouseButtonEvent(ImGuiMouseButton_Middle, false);
    });
    CHECK_FALSE(imgui.panel.wantsCursorLocked());
    CHECK(imgui.panel.isCameraMoving());
    imgui.settle();
    CHECK_FALSE(imgui.panel.isCameraMoving());

    const ViewRequest after = imgui.describe();
    const Vec3 right = Vec3(before.pose.world[0]);
    CHECK(glm::dot(positionOf(after) - positionOf(before), right) < -0.1f);
    CHECK(glm::length(forwardOf(after) - forwardOf(before)) == Approx(0.0f).margin(0.0001f));
}

TEST_CASE("the wheel zooms the scene view's camera, which asks for frames until the zoom arrives", "[editor][ui]") {
    HeadlessImGui imgui;
    const ViewRequest before = imgui.describe();
    CHECK_FALSE(imgui.panel.isCameraMoving());

    imgui.frame([](ImGuiIO& io) {
        io.AddMousePosEvent(IN_VIEW.x, IN_VIEW.y);
    });
    imgui.frame([](ImGuiIO& io) {
        io.AddMouseWheelEvent(0.0f, 1.0f);
    });
    CHECK(imgui.panel.isCameraMoving());

    imgui.settle();
    CHECK_FALSE(imgui.panel.isCameraMoving());
    const ViewRequest after = imgui.describe();
    const float travel = glm::dot(positionOf(after) - positionOf(before), forwardOf(before));
    CHECK(travel > 0.5f);
    CHECK(glm::length(forwardOf(after) - forwardOf(before)) == Approx(0.0f).margin(0.0001f));
}

TEST_CASE("the wheel does nothing while the mouse is outside the scene view", "[editor][ui]") {
    HeadlessImGui imgui;
    const ViewRequest before = imgui.describe();

    imgui.frame([](ImGuiIO& io) {
        io.AddMouseWheelEvent(0.0f, 1.0f);
    });
    imgui.settle();
    CHECK(glm::length(positionOf(imgui.describe()) - positionOf(before)) == Approx(0.0f).margin(0.0001f));
}

TEST_CASE("F flies to the selected entity while another panel has focus and no mouse button is held", "[editor][ui]") {
    HeadlessImGui imgui;
    const Vec3 target(8.0f, 0.0f, -12.0f);
    selectEntityAt(imgui.snapshot, target);
    imgui.otherWindowFocused = true;
    imgui.frame();
    imgui.frame();

    imgui.frame([](ImGuiIO& io) {
        io.AddKeyEvent(ImGuiKey_F, true);
    });
    CHECK(imgui.panel.isCameraMoving());
    imgui.frame([](ImGuiIO& io) {
        io.AddKeyEvent(ImGuiKey_F, false);
    });
    imgui.settle();

    const ViewRequest after = imgui.describe();
    CHECK(glm::dot(forwardOf(after), glm::normalize(target - positionOf(after))) == Approx(1.0f).margin(0.0001f));
    CHECK_FALSE(imgui.panel.isCameraMoving());
}

TEST_CASE("F does nothing with no selection", "[editor][ui]") {
    HeadlessImGui imgui;
    const ViewRequest before = imgui.describe();

    imgui.frame([](ImGuiIO& io) {
        io.AddKeyEvent(ImGuiKey_F, true);
    });
    imgui.frame([](ImGuiIO& io) {
        io.AddKeyEvent(ImGuiKey_F, false);
    });
    CHECK_FALSE(imgui.panel.isCameraMoving());
    CHECK(glm::length(positionOf(imgui.describe()) - positionOf(before)) == Approx(0.0f).margin(0.0001f));
}

static float zoomTravelWith(const ImGuiKey modifier) {
    HeadlessImGui imgui;
    const ViewRequest before = imgui.describe();
    imgui.frame([](ImGuiIO& io) {
        io.AddMousePosEvent(IN_VIEW.x, IN_VIEW.y);
    });
    if (modifier != ImGuiKey_None) {
        imgui.frame([modifier](ImGuiIO& io) {
            io.AddKeyEvent(modifier, true);
        });
    }
    imgui.frame([](ImGuiIO& io) {
        io.AddMouseWheelEvent(0.0f, 1.0f);
    });
    imgui.settle();
    return glm::dot(positionOf(imgui.describe()) - positionOf(before), forwardOf(before));
}

TEST_CASE("shift speeds up and ctrl slows down the scene view's zoom", "[editor][ui]") {
    const float normal = zoomTravelWith(ImGuiKey_None);
    REQUIRE(normal > 0.5f);
    CHECK(zoomTravelWith(ImGuiMod_Shift) == Approx(normal * TechEngine::FlyCamera::FAST_MULTIPLIER).margin(0.001f));
    CHECK(zoomTravelWith(ImGuiMod_Ctrl) == Approx(normal * TechEngine::FlyCamera::SLOW_MULTIPLIER).margin(0.001f));
}
