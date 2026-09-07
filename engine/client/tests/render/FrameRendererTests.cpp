#include <TechEngine/client/render/FrameCommand.hpp>
#include <TechEngine/platform/window/Window.hpp>

#include <render/FrameRenderer.hpp>

#include <catch2/catch_test_macros.hpp>
#include <glad/gl.h>

#include <array>
#include <cstddef>
#include <thread>

struct FrameRendererWindowScope {
    ~FrameRendererWindowScope() {
        TechEngine::Window::terminate();
    }
};

TEST_CASE("Frame renderer clears, draws, redraws and resizes on its context owner", "[client][render][window]") {
    const FrameRendererWindowScope scope;
    REQUIRE(TechEngine::Window::initialize());
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Frame renderer test"));

    constexpr int IMAGE_SIZE = 64;
    using Image = std::array<unsigned char, IMAGE_SIZE * IMAGE_SIZE * 4>;
    Image cleared{};
    Image triangle{};
    Image repeated{};
    Image minimized{};
    Image restored{};
    std::array<int, 4> initialViewport{};
    std::array<int, 4> resizedViewport{};
    bool loaded = false;
    bool complete = false;
    bool initialized = false;
    bool reinitialized = false;
    unsigned int error = GL_NO_ERROR;
    {
        std::jthread worker([&] {
            window.makeContextCurrent();
            const TechEngine::GlProcLoader loader = window.processLoader();
            loaded = loader != nullptr && gladLoadGL(loader) != 0 && GLAD_GL_VERSION_4_5 != 0;
            if (!loaded) {
                window.releaseContext();
                return;
            }

            unsigned int texture = 0;
            unsigned int framebuffer = 0;
            glCreateTextures(GL_TEXTURE_2D, 1, &texture);
            glTextureStorage2D(texture, 1, GL_RGBA8, IMAGE_SIZE, IMAGE_SIZE);
            glCreateFramebuffers(1, &framebuffer);
            glNamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0, texture, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

            TechEngine::FrameRenderer renderer;
            if (complete) {
                initialized = renderer.initialize();
            }
            if (initialized) {
                const TechEngine::FrameCommand clear{{1.0F, 0.0F, 0.0F, 1.0F}, false, 1};
                renderer.draw(clear, TechEngine::FramebufferSize{IMAGE_SIZE, IMAGE_SIZE});
                glReadPixels(0, 0, IMAGE_SIZE, IMAGE_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, cleared.data());
                glGetIntegerv(GL_VIEWPORT, initialViewport.data());

                const TechEngine::FrameCommand draw{{0.0F, 0.0F, 0.0F, 1.0F}, true, 2};
                renderer.draw(draw, TechEngine::FramebufferSize{IMAGE_SIZE, IMAGE_SIZE});
                renderer.draw(draw, TechEngine::FramebufferSize{32, 48});
                glReadPixels(0, 0, IMAGE_SIZE, IMAGE_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, triangle.data());
                glGetIntegerv(GL_VIEWPORT, resizedViewport.data());
                const std::array<float, 4> overwritten{1.0F, 0.0F, 1.0F, 1.0F};
                glClearBufferfv(GL_COLOR, 0, overwritten.data());
                renderer.draw(draw, TechEngine::FramebufferSize{32, 48});
                glReadPixels(0, 0, IMAGE_SIZE, IMAGE_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, repeated.data());

                const TechEngine::FrameCommand green{{0.0F, 1.0F, 0.0F, 1.0F}, false, 3};
                renderer.draw(green, TechEngine::FramebufferSize{0, 48});
                renderer.draw(green, TechEngine::FramebufferSize{32, 0});
                renderer.draw(green, TechEngine::FramebufferSize{0, 0});
                glReadPixels(0, 0, IMAGE_SIZE, IMAGE_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, minimized.data());
                renderer.shutdown();
                renderer.shutdown();
                reinitialized = renderer.initialize();
                if (reinitialized) {
                    renderer.draw(green, TechEngine::FramebufferSize{IMAGE_SIZE, IMAGE_SIZE});
                    glReadPixels(0, 0, IMAGE_SIZE, IMAGE_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, restored.data());
                }
            }
            renderer.shutdown();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &framebuffer);
            glDeleteTextures(1, &texture);
            error = glGetError();
            window.releaseContext();
        });
    }

    REQUIRE(loaded);
    REQUIRE(complete);
    REQUIRE(initialized);
    REQUIRE(reinitialized);
    CHECK(error == GL_NO_ERROR);
    const std::array<int, 4> expectedInitialViewport{0, 0, IMAGE_SIZE, IMAGE_SIZE};
    const std::array<int, 4> expectedResizedViewport{0, 0, 32, 48};
    CHECK(initialViewport == expectedInitialViewport);
    CHECK(resizedViewport == expectedResizedViewport);
    CHECK(repeated == triangle);
    CHECK(minimized == triangle);

    bool redClear = true;
    bool greenClear = true;
    std::size_t trianglePixels = 0;
    for (std::size_t i = 0; i < cleared.size(); i += 4) {
        redClear = redClear && cleared[i] == 255 && cleared[i + 1] == 0 && cleared[i + 2] == 0 && cleared[i + 3] == 255;
        greenClear = greenClear && restored[i] == 0 && restored[i + 1] == 255 && restored[i + 2] == 0 && restored[i + 3] == 255;
        if (triangle[i] != 0 || triangle[i + 1] != 0 || triangle[i + 2] != 0) {
            trianglePixels++;
        }
    }
    CHECK(redClear);
    CHECK(greenClear);
    CHECK(trianglePixels > 0);
    CHECK(trianglePixels < static_cast<std::size_t>(32 * 48));
}
