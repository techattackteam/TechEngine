#include <TechEngine/client/ClientError.hpp>
#include <TechEngine/client/render/ImGuiHook.hpp>
#include <TechEngine/client/render/RenderView.hpp>
#include <TechEngine/platform/window/Window.hpp>
#include <TechEngine/testing/WindowTerminateGuard.hpp>

#include <render/ViewTargets.hpp>
#include <render/device/RenderDevice.hpp>

#include <catch2/catch_test_macros.hpp>
#include <glad/gl.h>

#include <cstddef>
#include <cstdint>
#include <system_error>
#include <thread>
#include <vector>

using TechEngine::ImGuiViewTexture;
using TechEngine::RenderDevice;
using TechEngine::VIEW_TEXTURE_TOKEN_BIT;
using TechEngine::ViewRequest;
using TechEngine::ViewTarget;
using TechEngine::ViewTargets;

static constexpr std::uint64_t SCENE_TOKEN = VIEW_TEXTURE_TOKEN_BIT | 1;
static constexpr std::uint64_t GAME_TOKEN = VIEW_TEXTURE_TOKEN_BIT | 2;
// Two textures and the target that joins them.
static constexpr std::size_t OBJECTS_PER_VIEW = 3;

static PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC g_originalFramebufferStatus = nullptr;
static int g_failingFramebufferChecks = 0;

static GLenum GLAD_API_PTR failingFramebufferStatus(const GLuint framebuffer, const GLenum target) {
    if (g_failingFramebufferChecks > 0) {
        g_failingFramebufferChecks--;
        return GL_FRAMEBUFFER_UNSUPPORTED;
    }
    return g_originalFramebufferStatus(framebuffer, target);
}

// Construct it on the context's thread after glad has loaded, or the load overwrites it.
struct FailingFramebufferScope {
    explicit FailingFramebufferScope(const int failures) {
        g_failingFramebufferChecks = failures;
        g_originalFramebufferStatus = glad_glCheckNamedFramebufferStatus;
        glad_glCheckNamedFramebufferStatus = failingFramebufferStatus;
    }

    ~FailingFramebufferScope() {
        glad_glCheckNamedFramebufferStatus = g_originalFramebufferStatus;
        g_failingFramebufferChecks = 0;
    }
};

template<typename Body>
static bool runOnContext(TechEngine::Window& window, Body&& body) {
    bool loaded = false;
    {
        std::jthread worker([&] {
            window.makeContextCurrent();
            const TechEngine::GlProcLoader loader = window.processLoader();
            loaded = loader != nullptr && gladLoadGL(loader) != 0 && GLAD_GL_VERSION_4_6 != 0 && GLAD_GL_ARB_bindless_texture != 0;
            if (loaded) {
                body();
            }
            window.releaseContext();
        });
    }
    return loaded;
}

static ViewRequest requestOf(const std::uint64_t token, const int width, const int height) {
    ViewRequest request;
    request.token = token;
    request.width = width;
    request.height = height;
    request.visible = true;
    return request;
}

template<typename Body>
static bool runWithTargets(const char* title, Body&& body) {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, title) == std::error_code{});
    return runOnContext(window, [&] {
        RenderDevice device;
        ViewTargets targets;
        targets.initialize(device);
        body(device, targets);
        targets.shutdown();
        device.shutdown();
    });
}

TEST_CASE("view targets create one target per sized request, at its size", "[client][render][view][window][gpu]") {
    std::error_code updateError;
    bool sceneFound = false;
    bool gameFound = false;
    int sceneWidth = 0;
    int sceneHeight = 0;
    int gameWidth = 0;
    int gameHeight = 0;
    std::size_t liveObjects = 0;
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runWithTargets("View targets create test", [&](RenderDevice& device, ViewTargets& targets) {
        const std::vector<ViewRequest> requests{requestOf(SCENE_TOKEN, 640, 360), requestOf(GAME_TOKEN, 320, 200)};
        updateError = targets.update(requests);
        const ViewTarget* scene = targets.findTarget(SCENE_TOKEN);
        const ViewTarget* game = targets.findTarget(GAME_TOKEN);
        sceneFound = scene != nullptr && scene->target.valid();
        gameFound = game != nullptr && game->target.valid();
        if (sceneFound && gameFound) {
            sceneWidth = scene->extent.width;
            sceneHeight = scene->extent.height;
            gameWidth = game->extent.width;
            gameHeight = game->extent.height;
        }
        liveObjects = device.getLiveObjectCount();
        error = glGetError();
    });

    REQUIRE(loaded);
    CHECK_FALSE(updateError);
    REQUIRE(sceneFound);
    REQUIRE(gameFound);
    CHECK(sceneWidth == 640);
    CHECK(sceneHeight == 360);
    CHECK(gameWidth == 320);
    CHECK(gameHeight == 200);
    CHECK(liveObjects == 2 * OBJECTS_PER_VIEW);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("view targets replace a resized view's target and keep an unchanged one", "[client][render][view][window][gpu]") {
    std::error_code beforeError;
    std::error_code afterError;
    bool found = false;
    bool unchangedKept = false;
    int resizedWidth = 0;
    int resizedHeight = 0;
    std::size_t liveObjects = 0;
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runWithTargets("View targets resize test", [&](RenderDevice& device, ViewTargets& targets) {
        const std::vector<ViewRequest> before{requestOf(SCENE_TOKEN, 640, 360), requestOf(GAME_TOKEN, 320, 200)};
        const std::vector<ViewRequest> after{requestOf(SCENE_TOKEN, 800, 450), requestOf(GAME_TOKEN, 320, 200)};
        beforeError = targets.update(before);
        const ViewTarget* gameBefore = targets.findTarget(GAME_TOKEN);
        const TechEngine::RenderTargetHandle gameTargetBefore = gameBefore != nullptr ? gameBefore->target : TechEngine::RenderTargetHandle{};

        afterError = targets.update(after);
        const ViewTarget* scene = targets.findTarget(SCENE_TOKEN);
        const ViewTarget* game = targets.findTarget(GAME_TOKEN);
        found = gameBefore != nullptr && scene != nullptr && game != nullptr;
        if (!found) {
            return;
        }
        resizedWidth = scene->extent.width;
        resizedHeight = scene->extent.height;
        unchangedKept = game->target == gameTargetBefore;
        liveObjects = device.getLiveObjectCount();
        error = glGetError();
    });

    REQUIRE(loaded);
    CHECK_FALSE(beforeError);
    CHECK_FALSE(afterError);
    REQUIRE(found);
    CHECK(resizedWidth == 800);
    CHECK(resizedHeight == 450);
    CHECK(unchangedKept);
    CHECK(liveObjects == 2 * OBJECTS_PER_VIEW);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("view targets free a view that is no longer requested, and a zero-size view has none", "[client][render][view][window][gpu]") {
    std::error_code updateError;
    bool sceneGone = false;
    bool gameKept = false;
    bool zeroWidthHasNone = false;
    bool zeroHeightHasNone = false;
    std::size_t afterRemoval = 0;
    std::size_t afterZero = 0;
    std::size_t afterShutdown = 1;
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runWithTargets("View targets removal test", [&](RenderDevice& device, ViewTargets& targets) {
        const std::vector<ViewRequest> both{requestOf(SCENE_TOKEN, 640, 360), requestOf(GAME_TOKEN, 320, 200)};
        const std::vector<ViewRequest> gameOnly{requestOf(GAME_TOKEN, 320, 200)};
        const std::vector<ViewRequest> zeroWidth{requestOf(SCENE_TOKEN, 0, 360), requestOf(GAME_TOKEN, 320, 200)};
        const std::vector<ViewRequest> zeroHeight{requestOf(SCENE_TOKEN, 640, 0), requestOf(GAME_TOKEN, 320, 200)};
        const std::error_code bothError = targets.update(both);
        const std::error_code gameOnlyError = targets.update(gameOnly);
        sceneGone = targets.findTarget(SCENE_TOKEN) == nullptr;
        gameKept = targets.findTarget(GAME_TOKEN) != nullptr;
        afterRemoval = device.getLiveObjectCount();

        const std::error_code zeroWidthError = targets.update(zeroWidth);
        zeroWidthHasNone = targets.findTarget(SCENE_TOKEN) == nullptr;
        const std::error_code zeroHeightError = targets.update(zeroHeight);
        updateError = bothError ? bothError : (gameOnlyError ? gameOnlyError : (zeroWidthError ? zeroWidthError : zeroHeightError));
        zeroHeightHasNone = targets.findTarget(SCENE_TOKEN) == nullptr;
        afterZero = device.getLiveObjectCount();

        targets.shutdown();
        afterShutdown = device.getLiveObjectCount();
        error = glGetError();
    });

    REQUIRE(loaded);
    CHECK_FALSE(updateError);
    CHECK(sceneGone);
    CHECK(gameKept);
    CHECK(afterRemoval == OBJECTS_PER_VIEW);
    CHECK(zeroWidthHasNone);
    CHECK(zeroHeightHasNone);
    CHECK(afterZero == OBJECTS_PER_VIEW);
    CHECK(afterShutdown == 0);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("view targets name each target's colour texture for ImGui by its token", "[client][render][view][window][gpu]") {
    std::vector<ImGuiViewTexture> textures{ImGuiViewTexture{7, 7}};
    std::vector<ImGuiViewTexture> empty{ImGuiViewTexture{7, 7}};
    std::error_code updateError;
    bool found = false;
    std::uint64_t sceneColor = 0;
    std::uint64_t gameColor = 0;
    const bool loaded = runWithTargets("View targets ImGui test", [&](RenderDevice& device, ViewTargets& targets) {
        targets.collectImGuiTextures(empty);

        const std::vector<ViewRequest> requests{requestOf(SCENE_TOKEN, 64, 64), requestOf(GAME_TOKEN, 32, 32)};
        updateError = targets.update(requests);
        targets.collectImGuiTextures(textures);
        const ViewTarget* scene = targets.findTarget(SCENE_TOKEN);
        const ViewTarget* game = targets.findTarget(GAME_TOKEN);
        found = scene != nullptr && game != nullptr;
        if (!found) {
            return;
        }
        sceneColor = device.getNativeTexture(scene->color);
        gameColor = device.getNativeTexture(game->color);
    });

    REQUIRE(loaded);
    CHECK_FALSE(updateError);
    REQUIRE(found);
    CHECK(empty.empty());
    REQUIRE(textures.size() == 2);
    CHECK(sceneColor != 0);
    CHECK(gameColor != 0);
    CHECK(sceneColor != gameColor);
    for (const ImGuiViewTexture& texture: textures) {
        CHECK((texture.token == SCENE_TOKEN || texture.token == GAME_TOKEN));
        const std::uint64_t expected = texture.token == SCENE_TOKEN ? sceneColor : gameColor;
        CHECK(texture.nativeTexture == expected);
    }
    const bool distinctTokens = textures[0].token != textures[1].token;
    CHECK(distinctTokens);
}

TEST_CASE("view targets build every view they can when one fails, return the failure, and retry it on the next update", "[client][render][view][window][gpu]") {
    std::error_code firstError;
    std::error_code retryError;
    std::error_code resizeError;
    bool sceneMissing = false;
    bool gameBuilt = false;
    bool sceneRetried = false;
    bool gameDropped = false;
    bool sceneKept = false;
    std::size_t afterFailure = 0;
    std::size_t afterResizeFailure = 0;
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runWithTargets("View targets failure test", [&](RenderDevice& device, ViewTargets& targets) {
        const std::vector<ViewRequest> both{requestOf(SCENE_TOKEN, 64, 64), requestOf(GAME_TOKEN, 32, 32)};
        const std::vector<ViewRequest> gameResized{requestOf(SCENE_TOKEN, 64, 64), requestOf(GAME_TOKEN, 48, 48)};
        {
            const FailingFramebufferScope failing{1};
            firstError = targets.update(both);
        }
        sceneMissing = targets.findTarget(SCENE_TOKEN) == nullptr;
        gameBuilt = targets.findTarget(GAME_TOKEN) != nullptr;
        afterFailure = device.getLiveObjectCount();

        retryError = targets.update(both);
        sceneRetried = targets.findTarget(SCENE_TOKEN) != nullptr;

        {
            const FailingFramebufferScope failing{1};
            resizeError = targets.update(gameResized);
        }
        gameDropped = targets.findTarget(GAME_TOKEN) == nullptr;
        sceneKept = targets.findTarget(SCENE_TOKEN) != nullptr;
        afterResizeFailure = device.getLiveObjectCount();
        error = glGetError();
    });

    REQUIRE(loaded);
    CHECK(firstError == TechEngine::ClientError::IncompleteRenderTarget);
    CHECK(sceneMissing);
    CHECK(gameBuilt);
    CHECK(afterFailure == OBJECTS_PER_VIEW);
    CHECK_FALSE(retryError);
    CHECK(sceneRetried);
    CHECK(resizeError == TechEngine::ClientError::IncompleteRenderTarget);
    CHECK(gameDropped);
    CHECK(sceneKept);
    CHECK(afterResizeFailure == OBJECTS_PER_VIEW);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("view targets name no ImGui texture and shut down quietly before they are initialized", "[client][view]") {
    ViewTargets targets;
    std::vector<ImGuiViewTexture> textures{ImGuiViewTexture{SCENE_TOKEN, 7}};

    targets.collectImGuiTextures(textures);
    targets.shutdown();

    CHECK(textures.empty());
    CHECK(targets.findTarget(SCENE_TOKEN) == nullptr);
}
