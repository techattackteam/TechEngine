#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceUploadRecord.hpp>

#include <render/UploadChannel.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

using TechEngine::FinishedUpload;
using TechEngine::ResourceSlotId;
using TechEngine::ResourceUploadKind;
using TechEngine::ResourceUploadRecord;
using TechEngine::UploadChannel;

static ResourceUploadRecord recordWithToken(const std::uint64_t token, const ResourceUploadKind kind = ResourceUploadKind::Upload) {
    ResourceUploadRecord record;
    record.kind = kind;
    record.slot = ResourceSlotId{static_cast<std::uint32_t>(token), 0};
    record.token = kind == ResourceUploadKind::Upload ? token : 0;
    return record;
}

static std::vector<std::uint32_t> slotIndices(const std::vector<ResourceUploadRecord>& records) {
    std::vector<std::uint32_t> indices;
    for (const ResourceUploadRecord& record: records) {
        indices.push_back(record.slot.index);
    }
    return indices;
}

TEST_CASE("the upload channel delivers every record in the order sent, across several sends, and drops none", "[client][render][upload]") {
    UploadChannel channel;
    std::vector<ResourceUploadRecord> first{recordWithToken(1), recordWithToken(2)};
    std::vector<ResourceUploadRecord> second{recordWithToken(3, ResourceUploadKind::Release), recordWithToken(4)};

    channel.send(first);
    channel.send(second);

    std::vector<ResourceUploadRecord> received;
    channel.receive(received);
    REQUIRE(slotIndices(received) == std::vector<std::uint32_t>{1, 2, 3, 4});
    REQUIRE(received[2].kind == ResourceUploadKind::Release);

    std::vector<ResourceUploadRecord> again;
    channel.receive(again);
    REQUIRE(again.empty());
}

TEST_CASE("sending leaves the caller's list empty, and receiving replaces what the out list held", "[client][render][upload]") {
    UploadChannel channel;
    std::vector<ResourceUploadRecord> records{recordWithToken(1)};

    channel.send(records);

    REQUIRE(records.empty());
    std::vector<ResourceUploadRecord> received{recordWithToken(9)};
    channel.receive(received);
    REQUIRE(slotIndices(received) == std::vector<std::uint32_t>{1});
}

TEST_CASE("finished uploads come back once each, in the order reported, with their slots", "[client][render][upload]") {
    UploadChannel channel;
    const std::vector<FinishedUpload> firstBatch{FinishedUpload{ResourceSlotId{2, 0}, 5}, FinishedUpload{ResourceSlotId{1, 3}, 3}};
    const std::vector<FinishedUpload> secondBatch{FinishedUpload{ResourceSlotId{2, 0}, 8}};

    channel.reportFinished(firstBatch);
    channel.reportFinished(secondBatch);

    std::vector<FinishedUpload> finished;
    channel.takeFinished(finished);
    REQUIRE(finished == std::vector<FinishedUpload>{firstBatch[0], firstBatch[1], secondBatch[0]});

    std::vector<FinishedUpload> again;
    channel.takeFinished(again);
    REQUIRE(again.empty());
}

TEST_CASE("reset discards queued records and finished uploads", "[client][render][upload]") {
    UploadChannel channel;
    std::vector<ResourceUploadRecord> records{recordWithToken(1)};
    const std::vector<FinishedUpload> uploads{FinishedUpload{records[0].slot, 1}};
    channel.send(records);
    channel.reportFinished(uploads);

    channel.reset();

    std::vector<ResourceUploadRecord> received;
    channel.receive(received);
    REQUIRE(received.empty());
    std::vector<FinishedUpload> finished;
    channel.takeFinished(finished);
    REQUIRE(finished.empty());
}

TEST_CASE("records sent from one thread while another receives all arrive, in order", "[client][render][upload]") {
    constexpr std::uint64_t BATCH_COUNT = 1000;
    constexpr std::uint64_t BATCH_SIZE = 5;
    constexpr std::uint64_t RECORD_COUNT = BATCH_COUNT * BATCH_SIZE;
    UploadChannel channel;

    std::vector<ResourceUploadRecord> received;
    {
        const std::jthread sender{[&channel] {
            std::vector<ResourceUploadRecord> batch;
            for (std::uint64_t i = 0; i < BATCH_COUNT; i++) {
                for (std::uint64_t j = 0; j < BATCH_SIZE; j++) {
                    batch.push_back(recordWithToken(i * BATCH_SIZE + j + 1));
                }
                channel.send(batch);
            }
        }};

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        std::vector<ResourceUploadRecord> frame;
        while (received.size() < RECORD_COUNT && std::chrono::steady_clock::now() < deadline) {
            channel.receive(frame);
            received.insert(received.end(), frame.begin(), frame.end());
        }
    }

    REQUIRE(received.size() == RECORD_COUNT);
    for (std::uint64_t i = 0; i < RECORD_COUNT; i++) {
        REQUIRE(received[i].token == i + 1);
    }
}
