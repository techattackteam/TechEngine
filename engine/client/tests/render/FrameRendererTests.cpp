#include <TechEngine/platform/window/Window.hpp>
#include <TechEngine/testing/WindowTerminateGuard.hpp>

#include <render/FrameRenderer.hpp>
#include <render/FrameView.hpp>
#include <render/device/RenderDevice.hpp>
#include <render/device/RenderExtent.hpp>
#include <render/device/RenderHandle.hpp>
#include <render/device/target/RenderTarget.hpp>
#include <render/device/texture/Texture.hpp>

#include <catch2/catch_test_macros.hpp>
#include <glad/gl.h>

#include <array>
#include <cstddef>
#include <system_error>
#include <thread>

enum class FrameShaderFailure { VertexCompile, FragmentCompile, Link };

static TechEngine::FrameView frameView(const std::array<float, 4>& clearColor, const bool drawTriangle) {
    TechEngine::FrameView view;
    view.clearColor = clearColor;
    view.drawTriangle = drawTriangle;
    return view;
}

static PFNGLSHADERSOURCEPROC g_originalShaderSource = nullptr;
static FrameShaderFailure g_shaderFailure = FrameShaderFailure::VertexCompile;

static void GLAD_API_PTR failingShaderSource(unsigned int shader, int count, const char* const* source, const int* length) {
    int type = 0;
    glGetShaderiv(shader, GL_SHADER_TYPE, &type);
    const char* replacement = nullptr;
    if ((g_shaderFailure == FrameShaderFailure::VertexCompile && type == GL_VERTEX_SHADER) || (g_shaderFailure == FrameShaderFailure::FragmentCompile && type == GL_FRAGMENT_SHADER)) {
        replacement = "#version 450 core\ninvalid shader source";
    } else if (g_shaderFailure == FrameShaderFailure::Link) {
        replacement = type == GL_VERTEX_SHADER ? "#version 450 core\nout vec3 mismatch; void main() { mismatch = vec3(1.0); gl_Position = vec4(0.0, 0.0, 0.0, 1.0); }" : "#version 450 core\nin vec4 mismatch; out vec4 color; void main() { color = mismatch; }";
    }
    if (replacement != nullptr) {
        g_originalShaderSource(shader, 1, &replacement, nullptr);
    } else {
        g_originalShaderSource(shader, count, source, length);
    }
}

struct FrameShaderFailureScope {
    explicit FrameShaderFailureScope(FrameShaderFailure failure) {
        g_shaderFailure = failure;
        g_originalShaderSource = glad_glShaderSource;
        glad_glShaderSource = failingShaderSource;
    }

    ~FrameShaderFailureScope() {
        glad_glShaderSource = g_originalShaderSource;
    }
};

TEST_CASE("Frame renderer rejects shader failures and can retry initialization", "[client][render][window][gpu]") {
    FrameShaderFailure failure = FrameShaderFailure::VertexCompile;
    SECTION("Vertex compilation fails") {
        failure = FrameShaderFailure::VertexCompile;
    }
    SECTION("Fragment compilation fails") {
        failure = FrameShaderFailure::FragmentCompile;
    }
    SECTION("Shader interfaces fail to link") {
        failure = FrameShaderFailure::Link;
    }

    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Shader failure test") == std::error_code{});
    bool loaded = false;
    bool rejected = false;
    bool recovered = false;
    std::size_t leftAfterRejection = 0;
    std::size_t leftAfterShutdown = 0;
    unsigned int error = GL_NO_ERROR;
    {
        std::jthread worker([&] {
            window.makeContextCurrent();
            const TechEngine::GlProcLoader loader = window.processLoader();
            loaded = loader != nullptr && gladLoadGL(loader) != 0 && GLAD_GL_VERSION_4_6 != 0;
            if (loaded) {
                TechEngine::RenderDevice device;
                TechEngine::FrameRenderer renderer;
                {
                    const FrameShaderFailureScope failureScope{failure};
                    rejected = !renderer.initialize(device);
                }
                leftAfterRejection = device.getLiveObjectCount();
                recovered = renderer.initialize(device);
                renderer.shutdown();
                renderer.shutdown();
                leftAfterShutdown = device.getLiveObjectCount();
                device.shutdown();
                error = glGetError();
            }
            window.releaseContext();
        });
    }
    REQUIRE(loaded);
    CHECK(rejected);
    CHECK(leftAfterRejection == 0);
    CHECK(recovered);
    CHECK(leftAfterShutdown == 0);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("Frame renderer clears, draws, redraws and resizes through the device", "[client][render][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Frame renderer test") == std::error_code{});

    constexpr int IMAGE_SIZE = 64;
    constexpr int SMALL_WIDTH = 32;
    constexpr int SMALL_HEIGHT = 48;
    using Image = std::array<std::byte, static_cast<std::size_t>(IMAGE_SIZE) * IMAGE_SIZE * 4>;
    Image cleared{};
    Image triangle{};
    Image repeated{};
    Image minimized{};
    Image restored{};
    bool loaded = false;
    bool targetCreated = false;
    bool initialized = false;
    bool reinitialized = false;
    unsigned int error = GL_NO_ERROR;
    {
        std::jthread worker([&] {
            window.makeContextCurrent();
            const TechEngine::GlProcLoader loader = window.processLoader();
            loaded = loader != nullptr && gladLoadGL(loader) != 0 && GLAD_GL_VERSION_4_6 != 0;
            if (!loaded) {
                window.releaseContext();
                return;
            }

            TechEngine::RenderDevice device;
            TechEngine::TextureHandle color;
            TechEngine::RenderTargetHandle target;
            const std::error_code colorError = device.createTexture({IMAGE_SIZE, IMAGE_SIZE, 1, TechEngine::GpuTextureFormat::Rgba8}, color);
            const std::error_code targetError = device.createRenderTarget({color, TechEngine::TextureHandle{}}, target);
            targetCreated = !colorError && !targetError;

            TechEngine::FrameRenderer renderer;
            if (targetCreated) {
                initialized = renderer.initialize(device);
            }
            if (initialized) {
                const TechEngine::RenderExtent full{IMAGE_SIZE, IMAGE_SIZE};
                const TechEngine::RenderExtent small{SMALL_WIDTH, SMALL_HEIGHT};
                const TechEngine::FrameView clear = frameView({1.0F, 0.0F, 0.0F, 1.0F}, false);
                renderer.draw(clear, target, full);
                device.readTexture(color, cleared);

                const TechEngine::FrameView draw = frameView({0.0F, 0.0F, 0.0F, 1.0F}, true);
                renderer.draw(draw, target, small);
                device.readTexture(color, triangle);
                renderer.draw(draw, target, small);
                device.readTexture(color, repeated);

                const TechEngine::FrameView green = frameView({0.0F, 1.0F, 0.0F, 1.0F}, false);
                renderer.draw(green, target, TechEngine::RenderExtent{0, SMALL_HEIGHT});
                renderer.draw(green, target, TechEngine::RenderExtent{SMALL_WIDTH, 0});
                renderer.draw(green, target, TechEngine::RenderExtent{0, 0});
                device.readTexture(color, minimized);

                renderer.shutdown();
                renderer.shutdown();
                reinitialized = renderer.initialize(device);
                if (reinitialized) {
                    renderer.draw(green, target, full);
                    device.readTexture(color, restored);
                }
            }
            renderer.shutdown();
            device.shutdown();
            error = glGetError();
            window.releaseContext();
        });
    }

    REQUIRE(loaded);
    REQUIRE(targetCreated);
    REQUIRE(initialized);
    REQUIRE(reinitialized);
    CHECK(error == GL_NO_ERROR);
    const bool repeatedMatches = repeated == triangle;
    const bool minimizedMatches = minimized == triangle;
    CHECK(repeatedMatches);
    CHECK(minimizedMatches);

    bool redClear = true;
    bool greenClear = true;
    bool redOutsideSmall = true;
    std::size_t trianglePixels = 0;
    for (int y = 0; y < IMAGE_SIZE; y++) {
        for (int x = 0; x < IMAGE_SIZE; x++) {
            const std::size_t i = (static_cast<std::size_t>(y) * IMAGE_SIZE + static_cast<std::size_t>(x)) * 4;
            const auto channel = [](const Image& image, const std::size_t at) {
                return static_cast<unsigned char>(image[at]);
            };
            redClear = redClear && channel(cleared, i) == 255 && channel(cleared, i + 1) == 0 && channel(cleared, i + 2) == 0 && channel(cleared, i + 3) == 255;
            greenClear = greenClear && channel(restored, i) == 0 && channel(restored, i + 1) == 255 && channel(restored, i + 2) == 0 && channel(restored, i + 3) == 255;
            const bool insideSmall = x < SMALL_WIDTH && y < SMALL_HEIGHT;
            if (insideSmall && (channel(triangle, i) != 0 || channel(triangle, i + 1) != 0 || channel(triangle, i + 2) != 0)) {
                trianglePixels++;
            }
            if (!insideSmall) {
                redOutsideSmall = redOutsideSmall && channel(triangle, i) == 255 && channel(triangle, i + 1) == 0 && channel(triangle, i + 2) == 0;
            }
        }
    }
    CHECK(redClear);
    CHECK(greenClear);
    CHECK(redOutsideSmall);
    CHECK(trianglePixels > 0);
    CHECK(trianglePixels < static_cast<std::size_t>(SMALL_WIDTH * SMALL_HEIGHT));
}
