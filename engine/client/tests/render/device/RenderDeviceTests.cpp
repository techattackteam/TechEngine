#include <TechEngine/client/ClientError.hpp>
#include <TechEngine/platform/window/Window.hpp>
#include <TechEngine/testing/AssertCapture.hpp>
#include <TechEngine/testing/WindowTerminateGuard.hpp>

#include <render/device/RenderDevice.hpp>
#include <render/device/RenderExtent.hpp>
#include <render/device/RenderHandle.hpp>
#include <render/device/buffer/Buffer.hpp>
#include <render/device/pass/RenderPass.hpp>
#include <render/device/pipeline/Pipeline.hpp>
#include <render/device/pipeline/VertexLayout.hpp>
#include <render/device/program/Program.hpp>
#include <render/device/target/RenderTarget.hpp>
#include <render/device/texture/Texture.hpp>

#include <catch2/catch_test_macros.hpp>
#include <glad/gl.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <system_error>
#include <thread>
#include <vector>

static constexpr int TARGET_SIZE = 64;
static constexpr std::size_t IMAGE_BYTES = static_cast<std::size_t>(TARGET_SIZE) * TARGET_SIZE * 4;

static constexpr std::array<float, 9> TRIANGLE_VERTICES{-0.5F, -0.5F, 0.0F, 0.5F, -0.5F, 0.0F, 0.0F, 0.5F, 0.0F};
static constexpr std::array<std::uint32_t, 3> TRIANGLE_INDICES{0, 1, 2};

static constexpr const char* POSITION_VERTEX_SOURCE = R"(
    #version 460 core
    layout(location = 0) in vec3 position;
    void main() {
        gl_Position = vec4(position, 1.0);
    }
)";

static constexpr const char* WHITE_FRAGMENT_SOURCE = R"(
    #version 460 core
    out vec4 color;
    void main() {
        color = vec4(1.0);
    }
)";

static constexpr const char* STORAGE_FRAGMENT_SOURCE = R"(
    #version 460 core
    layout(std430, binding = 3) readonly buffer Tint {
        vec4 tint;
    };
    out vec4 color;
    void main() {
        color = tint;
    }
)";

static constexpr std::array<TechEngine::VertexAttribute, 1> POSITION_ATTRIBUTES{TechEngine::VertexAttribute{0, 0, TechEngine::VertexFormat::Float3, 0}};
static constexpr std::array<TechEngine::VertexBinding, 1> POSITION_BINDINGS{TechEngine::VertexBinding{0, static_cast<std::uint32_t>(3 * sizeof(float))}};

struct DevicePixel {
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned char a = 0;

    bool operator==(const DevicePixel&) const = default;
};

static constexpr DevicePixel RED{255, 0, 0, 255};
static constexpr DevicePixel BLUE{0, 0, 255, 255};
static constexpr DevicePixel BLACK{0, 0, 0, 255};
static constexpr DevicePixel WHITE{255, 255, 255, 255};
static constexpr DevicePixel GREEN{0, 255, 0, 255};

using Image = std::vector<std::byte>;

static DevicePixel pixelAt(const Image& image, const int x, const int y) {
    const std::size_t at = (static_cast<std::size_t>(y) * TARGET_SIZE + static_cast<std::size_t>(x)) * 4;
    return DevicePixel{static_cast<unsigned char>(image[at]), static_cast<unsigned char>(image[at + 1]), static_cast<unsigned char>(image[at + 2]), static_cast<unsigned char>(image[at + 3])};
}

static std::size_t countPixels(const Image& image, const DevicePixel color) {
    std::size_t count = 0;
    for (int y = 0; y < TARGET_SIZE; y++) {
        for (int x = 0; x < TARGET_SIZE; x++) {
            if (pixelAt(image, x, y) == color) {
                count++;
            }
        }
    }
    return count;
}

template<typename Body>
static bool runOnContext(TechEngine::Window& window, Body&& body) {
    bool loaded = false;
    {
        std::jthread worker([&] {
            window.makeContextCurrent();
            const TechEngine::GlProcLoader loader = window.processLoader();
            loaded = loader != nullptr && gladLoadGL(loader) != 0 && GLAD_GL_VERSION_4_6 != 0;
            if (loaded) {
                body();
            }
            window.releaseContext();
        });
    }
    return loaded;
}

struct DeviceTriangleScene {
    TechEngine::TextureHandle color;
    TechEngine::TextureHandle depth;
    TechEngine::RenderTargetHandle target;
    TechEngine::BufferHandle vertices;
    TechEngine::BufferHandle indices;
    TechEngine::ProgramHandle program;
    std::error_code error;

    void create(TechEngine::RenderDevice& device, const char* fragmentSource) {
        const std::error_code colorError = device.createTexture({TARGET_SIZE, TARGET_SIZE, 1, TechEngine::GpuTextureFormat::Rgba8}, color);
        const std::error_code depthError = device.createTexture({TARGET_SIZE, TARGET_SIZE, 1, TechEngine::GpuTextureFormat::Depth32F}, depth);
        const std::error_code targetError = device.createRenderTarget({color, depth}, target);
        const std::error_code vertexError = device.createBuffer({sizeof(TRIANGLE_VERTICES), TechEngine::BufferUsage::Static, std::as_bytes(std::span{TRIANGLE_VERTICES})}, vertices);
        const std::error_code indexError = device.createBuffer({sizeof(TRIANGLE_INDICES), TechEngine::BufferUsage::Static, std::as_bytes(std::span{TRIANGLE_INDICES})}, indices);
        const std::error_code programError = device.createProgram({POSITION_VERTEX_SOURCE, fragmentSource}, program);
        for (const std::error_code& step: {colorError, depthError, targetError, vertexError, indexError, programError}) {
            if (step && !error) {
                error = step;
            }
        }
    }

    TechEngine::PipelineDescription pipeline() const {
        TechEngine::PipelineDescription description;
        description.program = program;
        description.vertexLayout = TechEngine::VertexLayout{POSITION_ATTRIBUTES, POSITION_BINDINGS};
        return description;
    }

    void draw(TechEngine::RenderDevice& device, const TechEngine::PipelineHandle pipeline, const float clearDepth = 1.0F) const {
        TechEngine::RenderPassDescription pass;
        pass.target = target;
        pass.extent = TechEngine::RenderExtent{TARGET_SIZE, TARGET_SIZE};
        pass.clearColor = {0.0F, 0.0F, 0.0F, 1.0F};
        pass.clearDepth = clearDepth;
        device.beginRenderPass(pass);
        device.bindPipeline(pipeline);
        device.bindVertexBuffer(0, vertices, 0);
        device.bindIndexBuffer(indices);
        device.drawIndexed(3, 0, 0);
        device.endRenderPass();
    }

    Image read(const TechEngine::RenderDevice& device) const {
        Image image(IMAGE_BYTES);
        device.readTexture(color, image);
        return image;
    }
};

TEST_CASE("The device creates and destroys each kind of object, and shutdown frees what is left", "[client][render][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Device lifetime test") == std::error_code{});

    std::error_code setupError;
    std::error_code pipelineError;
    std::error_code dynamicError;
    std::size_t created = 0;
    std::size_t afterDestroy = 0;
    std::size_t afterShutdown = 0;
    std::size_t afterSecondShutdown = 0;
    bool handlesValid = false;
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runOnContext(window, [&] {
        TechEngine::RenderDevice device;
        DeviceTriangleScene scene;
        scene.create(device, WHITE_FRAGMENT_SOURCE);
        setupError = scene.error;
        TechEngine::PipelineHandle pipeline;
        pipelineError = device.createPipeline(scene.pipeline(), pipeline);
        TechEngine::BufferHandle dynamic;
        dynamicError = device.createBuffer({256, TechEngine::BufferUsage::Dynamic, {}}, dynamic);
        handlesValid = scene.color.valid() && scene.depth.valid() && scene.target.valid() && scene.vertices.valid() && scene.indices.valid() && scene.program.valid() && pipeline.valid() && dynamic.valid();
        created = device.getLiveObjectCount();

        device.destroyPipeline(pipeline);
        device.destroyBuffer(dynamic);
        afterDestroy = device.getLiveObjectCount();

        device.shutdown();
        afterShutdown = device.getLiveObjectCount();
        device.shutdown();
        afterSecondShutdown = device.getLiveObjectCount();
        error = glGetError();
    });

    REQUIRE(loaded);
    CHECK(setupError == std::error_code{});
    CHECK(pipelineError == std::error_code{});
    CHECK(dynamicError == std::error_code{});
    CHECK(handlesValid);
    CHECK(created == 8);
    CHECK(afterDestroy == 6);
    CHECK(afterShutdown == 0);
    CHECK(afterSecondShutdown == 0);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("A description the device cannot build is refused with InvalidRenderDescription and creates nothing", "[client][render][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Device description test") == std::error_code{});

    std::vector<std::error_code> results;
    bool anyHandleSet = false;
    std::size_t before = 0;
    std::size_t after = 0;
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runOnContext(window, [&] {
        TechEngine::RenderDevice device;
        TechEngine::ProgramHandle program;
        const std::error_code programError = device.createProgram({POSITION_VERTEX_SOURCE, WHITE_FRAGMENT_SOURCE}, program);
        TechEngine::TextureHandle small;
        const std::error_code smallError = device.createTexture({32, 32, 1, TechEngine::GpuTextureFormat::Rgba8}, small);
        TechEngine::TextureHandle color;
        const std::error_code colorError = device.createTexture({TARGET_SIZE, TARGET_SIZE, 1, TechEngine::GpuTextureFormat::Rgba8}, color);
        TechEngine::TextureHandle depth;
        const std::error_code depthError = device.createTexture({TARGET_SIZE, TARGET_SIZE, 1, TechEngine::GpuTextureFormat::Depth32F}, depth);
        TechEngine::TextureHandle stale;
        const std::error_code staleError = device.createTexture({TARGET_SIZE, TARGET_SIZE, 1, TechEngine::GpuTextureFormat::Rgba8}, stale);
        device.destroyTexture(stale);
        if (programError || smallError || colorError || depthError || staleError) {
            results.push_back(programError ? programError : smallError ? smallError : colorError ? colorError : depthError ? depthError : staleError);
            device.shutdown();
            return;
        }
        before = device.getLiveObjectCount();

        const std::array<std::byte, 8> bytes{};
        TechEngine::BufferHandle buffer;
        results.push_back(device.createBuffer({0, TechEngine::BufferUsage::Dynamic, {}}, buffer));
        results.push_back(device.createBuffer({16, TechEngine::BufferUsage::Static, {}}, buffer));
        results.push_back(device.createBuffer({16, TechEngine::BufferUsage::Static, bytes}, buffer));
        anyHandleSet = anyHandleSet || buffer.valid();

        TechEngine::TextureHandle texture;
        results.push_back(device.createTexture({0, TARGET_SIZE, 1, TechEngine::GpuTextureFormat::Rgba8}, texture));
        results.push_back(device.createTexture({TARGET_SIZE, -1, 1, TechEngine::GpuTextureFormat::Rgba8}, texture));
        results.push_back(device.createTexture({TARGET_SIZE, TARGET_SIZE, 0, TechEngine::GpuTextureFormat::Rgba8}, texture));
        anyHandleSet = anyHandleSet || texture.valid();

        TechEngine::PipelineHandle pipeline;
        TechEngine::PipelineDescription staleProgram;
        staleProgram.vertexLayout = TechEngine::VertexLayout{POSITION_ATTRIBUTES, POSITION_BINDINGS};
        results.push_back(device.createPipeline(staleProgram, pipeline));

        const std::array<TechEngine::VertexBinding, 1> tooHigh{TechEngine::VertexBinding{TechEngine::MAX_VERTEX_BINDINGS, 12}};
        const std::array<TechEngine::VertexAttribute, 1> onTooHigh{TechEngine::VertexAttribute{0, TechEngine::MAX_VERTEX_BINDINGS, TechEngine::VertexFormat::Float3, 0}};
        TechEngine::PipelineDescription bindingPastLimit;
        bindingPastLimit.program = program;
        bindingPastLimit.vertexLayout = TechEngine::VertexLayout{onTooHigh, tooHigh};
        results.push_back(device.createPipeline(bindingPastLimit, pipeline));

        const std::array<TechEngine::VertexAttribute, 1> undeclared{TechEngine::VertexAttribute{0, 1, TechEngine::VertexFormat::Float3, 0}};
        TechEngine::PipelineDescription undeclaredBinding;
        undeclaredBinding.program = program;
        undeclaredBinding.vertexLayout = TechEngine::VertexLayout{undeclared, POSITION_BINDINGS};
        results.push_back(device.createPipeline(undeclaredBinding, pipeline));
        anyHandleSet = anyHandleSet || pipeline.valid();

        TechEngine::RenderTargetHandle target;
        results.push_back(device.createRenderTarget({small, depth}, target));
        results.push_back(device.createRenderTarget({depth, TechEngine::TextureHandle{}}, target));
        results.push_back(device.createRenderTarget({color, color}, target));
        results.push_back(device.createRenderTarget({stale, depth}, target));
        anyHandleSet = anyHandleSet || target.valid();

        after = device.getLiveObjectCount();
        device.shutdown();
        error = glGetError();
    });

    REQUIRE(loaded);
    REQUIRE(results.size() == 13);
    for (const std::error_code& result: results) {
        CHECK(result == TechEngine::ClientError::InvalidRenderDescription);
    }
    CHECK_FALSE(anyHandleSet);
    CHECK(after == before);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("A shader that fails to compile or link returns its own error and creates nothing", "[client][render][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Device shader test") == std::error_code{});

    std::error_code vertexError;
    std::error_code fragmentError;
    std::error_code linkError;
    std::error_code recoveredError;
    bool failedHandleSet = false;
    std::size_t afterFailures = 0;
    std::size_t afterRecovery = 0;
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runOnContext(window, [&] {
        TechEngine::RenderDevice device;
        TechEngine::ProgramHandle program;
        vertexError = device.createProgram({"#version 460 core\ninvalid shader source", WHITE_FRAGMENT_SOURCE}, program);
        fragmentError = device.createProgram({POSITION_VERTEX_SOURCE, "#version 460 core\ninvalid shader source"}, program);
        linkError = device.createProgram({"#version 460 core\nout vec3 mismatch; void main() { mismatch = vec3(1.0); gl_Position = vec4(0.0, 0.0, 0.0, 1.0); }", "#version 460 core\nin vec4 mismatch; out vec4 color; void main() { color = mismatch; }"}, program);
        failedHandleSet = program.valid();
        afterFailures = device.getLiveObjectCount();
        recoveredError = device.createProgram({POSITION_VERTEX_SOURCE, WHITE_FRAGMENT_SOURCE}, program);
        afterRecovery = device.getLiveObjectCount();
        device.shutdown();
        error = glGetError();
    });

    REQUIRE(loaded);
    CHECK(vertexError == TechEngine::ClientError::ShaderCompileFailed);
    CHECK(fragmentError == TechEngine::ClientError::ShaderCompileFailed);
    CHECK(linkError == TechEngine::ClientError::ShaderLinkFailed);
    CHECK_FALSE(failedHandleSet);
    CHECK(afterFailures == 0);
    CHECK(recoveredError == std::error_code{});
    CHECK(afterRecovery == 1);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("A pass clears only its extent, and a Load pass keeps what the target held", "[client][render][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Device pass test") == std::error_code{});

    std::error_code setupError;
    Image cleared;
    Image corner;
    Image loadedAgain;
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runOnContext(window, [&] {
        TechEngine::RenderDevice device;
        DeviceTriangleScene scene;
        scene.create(device, WHITE_FRAGMENT_SOURCE);
        setupError = scene.error;
        if (!setupError) {
            TechEngine::RenderPassDescription red;
            red.target = scene.target;
            red.extent = TechEngine::RenderExtent{TARGET_SIZE, TARGET_SIZE};
            red.clearColor = {1.0F, 0.0F, 0.0F, 1.0F};
            device.beginRenderPass(red);
            device.endRenderPass();
            cleared = scene.read(device);

            TechEngine::RenderPassDescription blue = red;
            blue.extent = TechEngine::RenderExtent{16, 16};
            blue.clearColor = {0.0F, 0.0F, 1.0F, 1.0F};
            device.beginRenderPass(blue);
            device.endRenderPass();
            corner = scene.read(device);

            TechEngine::RenderPassDescription keep = red;
            keep.colorLoad = TechEngine::LoadAction::Load;
            device.beginRenderPass(keep);
            device.endRenderPass();
            loadedAgain = scene.read(device);
        }
        device.shutdown();
        error = glGetError();
    });

    REQUIRE(loaded);
    REQUIRE(setupError == std::error_code{});
    CHECK(countPixels(cleared, RED) == static_cast<std::size_t>(TARGET_SIZE * TARGET_SIZE));
    CHECK(countPixels(corner, BLUE) == 16 * 16);
    CHECK(countPixels(corner, RED) == static_cast<std::size_t>(TARGET_SIZE * TARGET_SIZE - 16 * 16));
    CHECK(pixelAt(corner, 0, 0) == BLUE);
    CHECK(pixelAt(corner, 15, 15) == BLUE);
    CHECK(pixelAt(corner, 16, 16) == RED);
    const bool loadKeptTheImage = loadedAgain == corner;
    CHECK(loadKeptTheImage);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("A triangle draws through a pipeline, its buffers and a pass", "[client][render][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Device triangle test") == std::error_code{});

    std::error_code setupError;
    std::error_code pipelineError;
    Image first;
    Image second;
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runOnContext(window, [&] {
        TechEngine::RenderDevice device;
        DeviceTriangleScene scene;
        scene.create(device, WHITE_FRAGMENT_SOURCE);
        setupError = scene.error;
        TechEngine::PipelineHandle pipeline;
        pipelineError = device.createPipeline(scene.pipeline(), pipeline);
        if (!setupError && !pipelineError) {
            scene.draw(device, pipeline);
            first = scene.read(device);
            scene.draw(device, pipeline);
            second = scene.read(device);
        }
        device.shutdown();
        error = glGetError();
    });

    REQUIRE(loaded);
    REQUIRE(setupError == std::error_code{});
    REQUIRE(pipelineError == std::error_code{});
    const std::size_t white = countPixels(first, WHITE);
    CHECK(white > 0);
    CHECK(white < static_cast<std::size_t>(TARGET_SIZE * TARGET_SIZE) / 2);
    CHECK(white + countPixels(first, BLACK) == static_cast<std::size_t>(TARGET_SIZE * TARGET_SIZE));
    CHECK(pixelAt(first, TARGET_SIZE / 2, TARGET_SIZE / 2) == WHITE);
    CHECK(pixelAt(first, 0, 0) == BLACK);
    CHECK(pixelAt(first, TARGET_SIZE - 1, TARGET_SIZE - 1) == BLACK);
    const bool redrawIsIdentical = second == first;
    CHECK(redrawIsIdentical);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("Each pipeline carries its own raster and depth state, bound whole", "[client][render][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Device pipeline state test") == std::error_code{});

    std::error_code setupError;
    std::error_code pipelineError;
    std::size_t culled = 0;
    std::size_t drawnAfterCull = 0;
    std::size_t culledAgain = 0;
    std::size_t depthRejected = 0;
    std::size_t depthIgnored = 0;
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runOnContext(window, [&] {
        TechEngine::RenderDevice device;
        DeviceTriangleScene scene;
        scene.create(device, WHITE_FRAGMENT_SOURCE);
        setupError = scene.error;

        TechEngine::PipelineDescription plainDescription = scene.pipeline();
        TechEngine::PipelineDescription cullingDescription = scene.pipeline();
        cullingDescription.cull = TechEngine::CullMode::Back;
        cullingDescription.frontFace = TechEngine::FrontFace::Clockwise;
        TechEngine::PipelineDescription depthDescription = scene.pipeline();
        depthDescription.depth = TechEngine::DepthState{true, true, TechEngine::CompareOp::Less};

        TechEngine::PipelineHandle plain;
        TechEngine::PipelineHandle culling;
        TechEngine::PipelineHandle depthTested;
        const std::error_code plainError = device.createPipeline(plainDescription, plain);
        const std::error_code cullingError = device.createPipeline(cullingDescription, culling);
        const std::error_code depthError = device.createPipeline(depthDescription, depthTested);
        pipelineError = plainError ? plainError : cullingError ? cullingError : depthError;

        if (!setupError && !pipelineError) {
            scene.draw(device, culling);
            culled = countPixels(scene.read(device), WHITE);
            scene.draw(device, plain);
            drawnAfterCull = countPixels(scene.read(device), WHITE);
            scene.draw(device, culling);
            culledAgain = countPixels(scene.read(device), WHITE);
            scene.draw(device, depthTested, 0.0F);
            depthRejected = countPixels(scene.read(device), WHITE);
            scene.draw(device, plain, 0.0F);
            depthIgnored = countPixels(scene.read(device), WHITE);
        }
        device.shutdown();
        error = glGetError();
    });

    REQUIRE(loaded);
    REQUIRE(setupError == std::error_code{});
    REQUIRE(pipelineError == std::error_code{});
    CHECK(culled == 0);
    CHECK(drawnAfterCull > 0);
    CHECK(culledAgain == 0);
    CHECK(depthRejected == 0);
    CHECK(depthIgnored == drawnAfterCull);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("A storage buffer binds at its binding, and a Dynamic buffer's update reaches the next draw", "[client][render][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Device storage buffer test") == std::error_code{});

    std::error_code setupError;
    std::error_code pipelineError;
    std::error_code tintError;
    DevicePixel before;
    DevicePixel after;
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runOnContext(window, [&] {
        TechEngine::RenderDevice device;
        DeviceTriangleScene scene;
        scene.create(device, STORAGE_FRAGMENT_SOURCE);
        setupError = scene.error;
        TechEngine::PipelineHandle pipeline;
        pipelineError = device.createPipeline(scene.pipeline(), pipeline);
        const std::array<float, 4> green{0.0F, 1.0F, 0.0F, 1.0F};
        const std::array<float, 4> red{1.0F, 0.0F, 0.0F, 1.0F};
        TechEngine::BufferHandle tint;
        tintError = device.createBuffer({sizeof(green), TechEngine::BufferUsage::Dynamic, std::as_bytes(std::span{green})}, tint);

        if (!setupError && !pipelineError && !tintError) {
            device.bindStorageBuffer(3, tint, 0, sizeof(green));
            scene.draw(device, pipeline);
            before = pixelAt(scene.read(device), TARGET_SIZE / 2, TARGET_SIZE / 2);
            device.updateBuffer(tint, 0, std::as_bytes(std::span{red}));
            scene.draw(device, pipeline);
            after = pixelAt(scene.read(device), TARGET_SIZE / 2, TARGET_SIZE / 2);
        }
        device.shutdown();
        error = glGetError();
    });

    REQUIRE(loaded);
    REQUIRE(setupError == std::error_code{});
    REQUIRE(pipelineError == std::error_code{});
    REQUIRE(tintError == std::error_code{});
    CHECK(before == GREEN);
    CHECK(after == RED);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("readTexture returns Rgba16F as floats and Depth32F as one float per pixel", "[client][render][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Device read formats test") == std::error_code{});

    constexpr int SIZE = 8;
    constexpr std::size_t PIXELS = static_cast<std::size_t>(SIZE) * SIZE;
    std::error_code setupError;
    std::vector<std::byte> colorBytes(PIXELS * TechEngine::getReadBytesPerPixel(TechEngine::GpuTextureFormat::Rgba16F));
    std::vector<std::byte> depthBytes(PIXELS * TechEngine::getReadBytesPerPixel(TechEngine::GpuTextureFormat::Depth32F));
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runOnContext(window, [&] {
        TechEngine::RenderDevice device;
        TechEngine::TextureHandle color;
        TechEngine::TextureHandle depth;
        TechEngine::RenderTargetHandle target;
        const std::error_code colorError = device.createTexture({SIZE, SIZE, 1, TechEngine::GpuTextureFormat::Rgba16F}, color);
        const std::error_code depthError = device.createTexture({SIZE, SIZE, 1, TechEngine::GpuTextureFormat::Depth32F}, depth);
        const std::error_code targetError = device.createRenderTarget({color, depth}, target);
        setupError = colorError ? colorError : depthError ? depthError : targetError;
        if (!setupError) {
            TechEngine::RenderPassDescription pass;
            pass.target = target;
            pass.extent = TechEngine::RenderExtent{SIZE, SIZE};
            pass.clearColor = {0.25F, 0.5F, 2.0F, 1.0F};
            pass.clearDepth = 0.75F;
            pass.depthStore = TechEngine::StoreAction::Store;
            device.beginRenderPass(pass);
            device.endRenderPass();
            device.readTexture(color, colorBytes);
            device.readTexture(depth, depthBytes);
        }
        device.shutdown();
        error = glGetError();
    });

    REQUIRE(loaded);
    REQUIRE(setupError == std::error_code{});
    CHECK(colorBytes.size() == PIXELS * 16);
    CHECK(depthBytes.size() == PIXELS * 4);
    bool colorMatches = true;
    bool depthMatches = true;
    for (std::size_t i = 0; i < PIXELS; i++) {
        std::array<float, 4> rgba{};
        std::memcpy(rgba.data(), colorBytes.data() + i * 16, 16);
        colorMatches = colorMatches && rgba[0] == 0.25F && rgba[1] == 0.5F && rgba[2] == 2.0F && rgba[3] == 1.0F;
        float value = 0.0F;
        std::memcpy(&value, depthBytes.data() + i * 4, 4);
        depthMatches = depthMatches && value == 0.75F;
    }
    CHECK(colorMatches);
    CHECK(depthMatches);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("A stale handle or a call out of order is caught by a check and reaches no GL call", "[client][render][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Device misuse test") == std::error_code{});

    std::error_code setupError;
    std::vector<std::size_t> firedAfterEachStep;
    std::uint64_t staleNative = 1;
    unsigned int error = GL_NO_ERROR;
    const TechEngineTests::AssertHandlerGuard asserts;
    const bool loaded = runOnContext(window, [&] {
        TechEngine::RenderDevice device;
        DeviceTriangleScene scene;
        scene.create(device, WHITE_FRAGMENT_SOURCE);
        setupError = scene.error;
        TechEngine::PipelineHandle pipeline;
        const std::error_code pipelineError = device.createPipeline(scene.pipeline(), pipeline);
        if (setupError || pipelineError) {
            device.shutdown();
            return;
        }
        const auto record = [&] {
            firedAfterEachStep.push_back(TechEngineTests::g_fired.size());
        };
        record();

        device.bindPipeline(pipeline);
        record();
        device.endRenderPass();
        record();
        const std::array<float, 4> bytes{};
        device.updateBuffer(scene.vertices, 0, std::as_bytes(std::span{bytes}));
        record();

        TechEngine::RenderPassDescription pass;
        pass.target = scene.target;
        pass.extent = TechEngine::RenderExtent{TARGET_SIZE, TARGET_SIZE};
        device.beginRenderPass(pass);
        record();
        device.drawIndexed(3, 0, 0);
        record();
        device.beginRenderPass(pass);
        record();
        device.endRenderPass();
        record();

        TechEngine::RenderPassDescription tooLarge = pass;
        tooLarge.extent = TechEngine::RenderExtent{TARGET_SIZE + 1, TARGET_SIZE};
        device.beginRenderPass(tooLarge);
        record();

        const TechEngine::BufferHandle stale = scene.indices;
        device.destroyBuffer(stale);
        record();
        device.destroyBuffer(stale);
        record();
        device.beginRenderPass(pass);
        device.bindPipeline(pipeline);
        device.bindIndexBuffer(stale);
        record();
        device.endRenderPass();

        TechEngine::TextureHandle staleTexture;
        const std::error_code textureError = device.createTexture({TARGET_SIZE, TARGET_SIZE, 1, TechEngine::GpuTextureFormat::Rgba8}, staleTexture);
        if (!textureError) {
            device.destroyTexture(staleTexture);
            staleNative = device.getNativeTexture(staleTexture);
        }
        record();

        device.shutdown();
        error = glGetError();
    });

    REQUIRE(loaded);
    REQUIRE(setupError == std::error_code{});
    REQUIRE(firedAfterEachStep.size() == 13);
    CHECK(firedAfterEachStep[0] == 0);
    CHECK(firedAfterEachStep[1] == 1);
    CHECK(firedAfterEachStep[2] == 2);
    CHECK(firedAfterEachStep[3] == 3);
    CHECK(firedAfterEachStep[4] == 3);
    CHECK(firedAfterEachStep[5] == 4);
    CHECK(firedAfterEachStep[6] == 5);
    CHECK(firedAfterEachStep[7] == 5);
    CHECK(firedAfterEachStep[8] == firedAfterEachStep[7] + 1);
    CHECK(firedAfterEachStep[9] == firedAfterEachStep[8]);
    CHECK(firedAfterEachStep[10] == firedAfterEachStep[9] + 1);
    CHECK(firedAfterEachStep[11] == firedAfterEachStep[10] + 1);
    CHECK(firedAfterEachStep[12] == firedAfterEachStep[11] + 1);
    CHECK(staleNative == 0);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("An asset texture takes every level, is made resident once, and is refused as a render target's colour", "[client][render][window][gpu]") {
    const TechEngineTests::AssertHandlerGuard guard;
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Device asset texture test") == std::error_code{});

    constexpr int SIZE = 8;
    constexpr int LEVELS = 4;
    constexpr std::size_t BC7_BLOCK_BYTES = 16;
    std::error_code createError;
    std::error_code targetError;
    std::uint64_t handleBeforeResidency = 1;
    std::uint64_t handle = 0;
    std::uint64_t handleAfterSecondCall = 0;
    bool resident = false;
    std::size_t checksAfterUpload = 0;
    std::size_t checksAfterBadLevel = 0;
    std::size_t checksAfterBadSize = 0;
    std::size_t checksAfterBadFormat = 0;
    std::size_t checksAfterSecondCall = 0;
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runOnContext(window, [&] {
        TechEngine::RenderDevice device;
        TechEngine::TextureHandle texture;
        createError = device.createTexture({SIZE, SIZE, LEVELS, TechEngine::GpuTextureFormat::Bc7Srgb}, texture);
        if (!createError) {
            for (std::uint32_t level = 0; level < LEVELS; level++) {
                const int side = std::max(1, SIZE >> level);
                const std::size_t blocks = static_cast<std::size_t>((side + 3) / 4) * static_cast<std::size_t>((side + 3) / 4);
                const std::vector<std::byte> bytes(blocks * BC7_BLOCK_BYTES);
                device.updateTexture(texture, level, bytes);
            }
            checksAfterUpload = TechEngineTests::g_fired.size();
            const std::vector<std::byte> block(BC7_BLOCK_BYTES);
            device.updateTexture(texture, LEVELS, block);
            checksAfterBadLevel = TechEngineTests::g_fired.size();
            device.updateTexture(texture, 0, {});
            checksAfterBadSize = TechEngineTests::g_fired.size();
            TechEngine::TextureHandle runtimeTexture;
            device.createTexture({SIZE, SIZE, 1, TechEngine::GpuTextureFormat::Rgba8}, runtimeTexture);
            device.updateTexture(runtimeTexture, 0, std::vector<std::byte>(SIZE * SIZE * 4));
            checksAfterBadFormat = TechEngineTests::g_fired.size();
            device.destroyTexture(runtimeTexture);
            handleBeforeResidency = device.getBindlessHandle(texture);
            device.makeTextureResident(texture);
            handle = device.getBindlessHandle(texture);
            resident = handle != 0 && glIsTextureHandleResidentARB(handle) == GL_TRUE;
            device.makeTextureResident(texture);
            checksAfterSecondCall = TechEngineTests::g_fired.size();
            handleAfterSecondCall = device.getBindlessHandle(texture);
            TechEngine::RenderTargetHandle target;
            targetError = device.createRenderTarget({texture, TechEngine::TextureHandle{}}, target);
        }
        device.shutdown();
        error = glGetError();
    });

    REQUIRE(loaded);
    REQUIRE(createError == std::error_code{});
    CHECK(checksAfterUpload == 0);
    CHECK(checksAfterBadLevel == 1);
    CHECK(checksAfterBadSize == 2);
    CHECK(checksAfterBadFormat == 3);
    CHECK(handleBeforeResidency == 0);
    CHECK(resident);
    CHECK(checksAfterSecondCall == 4);
    CHECK(handleAfterSecondCall == handle);
    CHECK(targetError == TechEngine::ClientError::InvalidRenderDescription);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("A signalled frame is reported complete once the GPU has finished it, and shutdown starts the count again", "[client][render][window][gpu]") {
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Device frame fence test") == std::error_code{});

    std::uint64_t beforeAny = 99;
    std::uint64_t afterFirst = 0;
    std::uint64_t afterThird = 0;
    std::uint64_t afterShutdown = 99;
    std::uint64_t afterRestart = 0;
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runOnContext(window, [&] {
        TechEngine::RenderDevice device;
        beforeAny = device.getCompletedFrame();
        device.signalFrame(1);
        glFinish();
        afterFirst = device.getCompletedFrame();
        device.signalFrame(2);
        device.signalFrame(3);
        glFinish();
        afterThird = device.getCompletedFrame();

        device.signalFrame(4);
        device.shutdown();
        afterShutdown = device.getCompletedFrame();
        device.signalFrame(1);
        glFinish();
        afterRestart = device.getCompletedFrame();
        device.shutdown();
        error = glGetError();
    });

    REQUIRE(loaded);
    CHECK(beforeAny == 0);
    CHECK(afterFirst == 1);
    CHECK(afterThird == 3);
    CHECK(afterShutdown == 0);
    CHECK(afterRestart == 1);
    CHECK(error == GL_NO_ERROR);
}

TEST_CASE("Signalling a frame that does not rise is caught by a check and inserts no fence", "[client][render][window][gpu]") {
    const TechEngineTests::AssertHandlerGuard guard;
    const TechEngineTests::WindowTerminateGuard scope;
    REQUIRE(TechEngine::Window::initialize() == std::error_code{});
    TechEngine::Window window;
    REQUIRE(window.open(320, 240, "Device frame fence misuse test") == std::error_code{});

    std::size_t checksAfterRising = 99;
    std::size_t checksAfterRepeat = 0;
    std::size_t checksAfterLower = 0;
    std::uint64_t completed = 0;
    unsigned int error = GL_NO_ERROR;
    const bool loaded = runOnContext(window, [&] {
        TechEngine::RenderDevice device;
        device.signalFrame(1);
        device.signalFrame(2);
        checksAfterRising = TechEngineTests::g_fired.size();
        device.signalFrame(2);
        checksAfterRepeat = TechEngineTests::g_fired.size();
        device.signalFrame(1);
        checksAfterLower = TechEngineTests::g_fired.size();
        glFinish();
        completed = device.getCompletedFrame();
        device.shutdown();
        error = glGetError();
    });

    REQUIRE(loaded);
    CHECK(checksAfterRising == 0);
    CHECK(checksAfterRepeat == 1);
    CHECK(checksAfterLower == 2);
    CHECK(completed == 2);
    CHECK(error == GL_NO_ERROR);
}
