#include <TechEngine/client/render/FrameCommand.hpp>

#include <render/FrameCommandBuffer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <barrier>
#include <cstdint>
#include <thread>

static void checkCommand(const TechEngine::FrameCommand& actual, const TechEngine::FrameCommand& expected) {
    CHECK(actual.clearColor == expected.clearColor);
    CHECK(actual.drawTriangle == expected.drawTriangle);
    CHECK(actual.frameIndex == expected.frameIndex);
}

TEST_CASE("Frame command buffer starts with an empty frame and resets between sessions", "[client][render][command-buffer]") {
    TechEngine::FrameCommandBuffer commandBuffer;
    const TechEngine::FrameCommand empty;
    checkCommand(commandBuffer.snapshot(), empty);

    const TechEngine::FrameCommand command{{0.25F, 0.5F, 0.75F, 1.0F}, true, 7};
    commandBuffer.publish(command);
    checkCommand(commandBuffer.snapshot(), command);
    commandBuffer.reset();
    checkCommand(commandBuffer.snapshot(), empty);
    commandBuffer.reset();
    checkCommand(commandBuffer.snapshot(), empty);
    commandBuffer.publish(command);
    checkCommand(commandBuffer.snapshot(), command);
}

TEST_CASE("Frame command buffer keeps the newest publication without draining it", "[client][render][command-buffer]") {
    TechEngine::FrameCommandBuffer commandBuffer;
    const TechEngine::FrameCommand older{{1.0F, 0.0F, 0.0F, 1.0F}, false, 100};
    const TechEngine::FrameCommand newer{{0.0F, 1.0F, 0.0F, 1.0F}, true, 101};
    commandBuffer.publish(older);
    commandBuffer.publish(newer);
    checkCommand(commandBuffer.snapshot(), newer);
    checkCommand(commandBuffer.snapshot(), newer);

    // Publication order wins even when a frame index is reused.
    const TechEngine::FrameCommand replacement{{0.0F, 0.0F, 1.0F, 1.0F}, false, 101};
    commandBuffer.publish(replacement);
    checkCommand(commandBuffer.snapshot(), replacement);
}

TEST_CASE("Frame command buffer owns its command and returns independent snapshots", "[client][render][command-buffer]") {
    TechEngine::FrameCommandBuffer commandBuffer;
    TechEngine::FrameCommand source{{0.25F, 0.5F, 0.75F, 1.0F}, true, 42};
    const TechEngine::FrameCommand expected = source;
    commandBuffer.publish(source);
    source = {};
    checkCommand(commandBuffer.snapshot(), expected);

    TechEngine::FrameCommand snapshot = commandBuffer.snapshot();
    snapshot.clearColor[0] = 1.0F;
    snapshot.drawTriangle = false;
    snapshot.frameIndex = 99;
    checkCommand(commandBuffer.snapshot(), expected);
    CHECK(snapshot.frameIndex == 99);
    checkCommand(source, TechEngine::FrameCommand{});
}

TEST_CASE("Frame command buffer transfers coherent commands between concurrent producer and consumer", "[client][render][command-buffer][threading]") {
    TechEngine::FrameCommandBuffer commandBuffer;
    constexpr std::uint64_t PUBLICATION_COUNT = 20000;
    std::barrier start{2};
    bool coherent = true;
    {
        std::jthread producer([&] {
            start.arrive_and_wait();
            for (std::uint64_t i = 1; i <= PUBLICATION_COUNT; i++) {
                const float value = static_cast<float>(i);
                commandBuffer.publish(TechEngine::FrameCommand{{value, value, value, value}, (i % 2) != 0, i});
            }
        });
        std::jthread consumer([&] {
            start.arrive_and_wait();
            std::uint64_t previousIndex = 0;
            for (std::uint64_t i = 0; i < PUBLICATION_COUNT; i++) {
                const TechEngine::FrameCommand command = commandBuffer.snapshot();
                const float value = static_cast<float>(command.frameIndex);
                const std::array<float, 4> expected{value, value, value, value};
                if (command.clearColor != expected || command.drawTriangle != ((command.frameIndex % 2) != 0) || command.frameIndex < previousIndex || command.frameIndex > PUBLICATION_COUNT) {
                    coherent = false;
                }
                previousIndex = command.frameIndex;
            }
        });
    }
    CHECK(coherent);
    const float finalValue = static_cast<float>(PUBLICATION_COUNT);
    checkCommand(commandBuffer.snapshot(), TechEngine::FrameCommand{{finalValue, finalValue, finalValue, finalValue}, false, PUBLICATION_COUNT});
}
