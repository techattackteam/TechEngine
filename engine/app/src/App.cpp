#include <TechEngine/app/App.hpp>
#include <TechEngine/app/FrameLoop.hpp>
#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/core/EngineContext.hpp>
#include <TechEngine/core/events/EventRegistry.hpp>
#include <TechEngine/core/events/EventStreamManager.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/MountTable.hpp>

#include <diagnostics/Diagnostics.hpp>
#include <diagnostics/MemoryTracking.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <thread>
#include <vector>

namespace TechEngine {
    struct DemoDamage {
        std::uint64_t tick = 0;
        std::uint32_t amount = 0;
    };

    struct DemoTransform {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float scale = 1.0f;

        bool operator==(const DemoTransform&) const = default;
    };

    struct DemoMaterial {
        StringId shader;
        std::string name;
        DemoTransform transform;
        std::vector<std::uint32_t> textureIds;

        bool operator==(const DemoMaterial&) const = default;
    };

    template<typename Archive>
    static void visit(Archive& archive, DemoTransform& value) {
        static_assert(sizeof(DemoTransform) == 16, "DemoTransform changed shape; check that visit still covers every field.");

        archive.field(value.x);
        archive.field(value.y);
        archive.field(value.z);
        archive.field(value.scale);
    }

    // No sizeof guard: a type holding std::string or std::vector has a different sizeof per
    // standard library and per config, so pinning a constant here goes red on another leg.
    template<typename Archive>
    static void visit(Archive& archive, DemoMaterial& value) {
        archive.field(value.shader);
        archive.field(value.name);
        archive.field(value.transform);
        archive.field(value.textureIds);
    }

    static constexpr const char* DEMO_BLOB_VIRTUAL_PATH = "assets://demo-material.bin";

    static constexpr std::uint64_t DEMO_DAMAGE_TICK_PERIOD = 30;
    static constexpr std::size_t DEMO_JOB_COUNT = 4;
    static constexpr std::uint64_t DEMO_SUM_LIMIT = 1000000;
    static constexpr std::uint64_t DEMO_SUM_EXPECTED = DEMO_SUM_LIMIT * (DEMO_SUM_LIMIT + 1) / 2;

    int run() {
        const DiagnosticsScope diagnostics;

        memoryTrackingAnchor();

        MountTable mounts;
        FileAccess files{mounts};
        JobSystem jobs;
        const EngineContext engine{files, jobs};

        // TODO(S3-T13): throwaway demo mount. M3 brings the real set — project root,
        // resources, cache — sourced from the loaded project, and the baked-in source path
        // goes with it.
        mounts.mount("assets", TE_DEMO_ASSETS_DIR);

        std::vector<std::byte> demoAsset;
        const FileResult demoRead = engine.files.read("assets://demo.txt", demoAsset);
        TE_LOGGER_INFO("assets://demo.txt -> result {0}, {1} bytes", static_cast<std::uint32_t>(demoRead), demoAsset.size());

        // TODO(S4-T7): throwaway serialization demo. M5 registers real components against this
        // seam; the struct below only proves the round-trip.
        DemoMaterial demoMaterial;
        demoMaterial.shader = StringId{"TechEngine.DemoUnlit"};
        demoMaterial.name = "materials/brushed-metal";
        demoMaterial.transform = DemoTransform{1.0f, 2.0f, 3.0f, 0.5f};
        demoMaterial.textureIds = {11, 22, 33};

        std::vector<std::byte> demoBlob;
        Writer demoWriter{demoBlob};
        demoWriter.writeHeader();
        demoWriter.field(demoMaterial);

        const FileResult demoBlobWritten = engine.files.write(DEMO_BLOB_VIRTUAL_PATH, demoBlob);

        std::vector<std::byte> demoBlobFromDisk;
        const FileResult demoBlobRead = engine.files.read(DEMO_BLOB_VIRTUAL_PATH, demoBlobFromDisk);

        Reader demoReader{demoBlobFromDisk};
        BlobHeader demoHeader;
        demoReader.readHeader(demoHeader);

        DemoMaterial demoRestored;
        demoReader.field(demoRestored);

        TE_LOGGER_INFO(
            "DemoMaterial: wrote {0} bytes to {1} (result {2}), read back result {3} at {4} bytes, header version {5}, status {6}, equal {7}, shader {8}, name {9}, textures {10}",
            demoBlob.size(),
            DEMO_BLOB_VIRTUAL_PATH,
            static_cast<std::uint32_t>(demoBlobWritten),
            static_cast<std::uint32_t>(demoBlobRead),
            demoBlobFromDisk.size(),
            demoHeader.formatVersion,
            static_cast<std::uint32_t>(demoReader.status()),
            demoRestored == demoMaterial,
            demoRestored.shader,
            demoRestored.name,
            demoRestored.textureIds.size());

        TE_ASSERT(demoBlobWritten == FileResult::Ok, "DemoMaterial blob did not write to {0}", DEMO_BLOB_VIRTUAL_PATH);
        TE_ASSERT(demoBlobRead == FileResult::Ok, "DemoMaterial blob did not read back from {0}", DEMO_BLOB_VIRTUAL_PATH);
        TE_ASSERT(demoReader.ok() && demoRestored == demoMaterial, "DemoMaterial did not survive the round-trip through disk");

        Clock clock;
        FrameLoop loop(engine, Role::Client);

        EventRegistry registry;
        registry.registerEvent<DemoDamage>("TechEngine.DemoDamage");

        EventStreamManager streams{registry};
        EventCursor damageCursor;

        std::array<std::uint64_t, DEMO_JOB_COUNT> demoResults{};
        std::vector<Task> demoTasks;
        demoTasks.reserve(DEMO_JOB_COUNT);

        Clock::TimePoint previous = clock.now();
        constexpr std::uint64_t frames = 120;

        constexpr Clock::TimePoint::duration frameBudget = std::chrono::duration_cast<Clock::TimePoint::duration>(std::chrono::duration<double>(FrameLoop::FIXED_DELTA_TIME));

        for (std::uint64_t i = 0; i < frames; ++i) {
            clock.advanceFrame();
            setDiagnosticFrame(clock.frame());

            const Clock::TimePoint current = clock.now();
            const double frameDeltaTime = std::chrono::duration<double>(current - previous).count();
            previous = current;

            loop.advance(frameDeltaTime, [&streams](const FrameContext& fixedStep) {
                if (fixedStep.tick % DEMO_DAMAGE_TICK_PERIOD == 0) {
                    streams.publish(DemoDamage{fixedStep.tick, 42});
                }
                streams.makeVisible(fixedStep.frameIndex, fixedStep.tick);
            });

            const FrameContext& frame = loop.frame();
            streams.makeVisible(frame.frameIndex, frame.tick);

            const std::span<const DemoDamage> damage = streams.read<DemoDamage>(damageCursor);
            for (const DemoDamage& event: damage) {
                TE_LOGGER_WARN("DemoDamage {0} from tick {1}, read on frame {2} ({3} in this batch)", event.amount, event.tick, frame.frameIndex, damage.size());
            }

            streams.retire(frame.frameIndex, frame.tick);

            // TODO(S4-T5): throwaway demo batch. It splits 1..DEMO_SUM_LIMIT across the pool
            demoTasks.clear();
            constexpr std::uint64_t demoChunk = DEMO_SUM_LIMIT / DEMO_JOB_COUNT;
            for (std::size_t j = 0; j < DEMO_JOB_COUNT; j++) {
                const std::uint64_t begin = j * demoChunk + 1;
                const std::uint64_t end = j + 1 == DEMO_JOB_COUNT ? DEMO_SUM_LIMIT : (j + 1) * demoChunk;
                demoTasks.emplace_back([&demoResults, j, begin, end] {
                    std::uint64_t sum = 0;
                    for (std::uint64_t k = begin; k <= end; k++) {
                        sum += k;
                    }
                    demoResults[j] = sum;
                });
            }

            const BatchId demoBatch = engine.jobs.submit(demoTasks);
            engine.jobs.wait(demoBatch);

            std::uint64_t demoTotal = 0;
            for (const std::uint64_t partial: demoResults) {
                demoTotal += partial;
            }
            TE_ASSERT(demoTotal == DEMO_SUM_EXPECTED, "Demo batch summed to {0}, expected {1}", demoTotal, DEMO_SUM_EXPECTED);

            TE_LOGGER_INFO(
                "Frame {0}: deltaTime = {1:.6f}, fixedDeltaTime = {2:.6f}, accumulator = {3:.6f}, tick = {4}, role = {5}, workers = {6}, jobSum = {7} of {8}",
                frame.frameIndex,
                frame.deltaTime,
                frame.fixedDeltaTime,
                loop.accumulator(),
                frame.tick,
                static_cast<std::uint32_t>(frame.role),
                engine.jobs.workerCount(),
                demoTotal,
                DEMO_SUM_EXPECTED);

            // TODO(S3): throwaway 60 Hz pacer. Spins because sleep_for rounds up to the
            // ~15.6 ms Windows timer tick; real frame pacing is undecided.
            const Clock::TimePoint frameDeadline = current + frameBudget;
            while (clock.now() < frameDeadline) {
                std::this_thread::yield();
            }

            TE_PROFILER_FRAME();
        }
        Vec3 position = Vec3(1.4f, 3, 4);
        Mat4 matrix = Mat4(1, 2, 3, 4, 5, 67, 8, 9, 1, 2, 3, 54, 6, 1, 2, 2);
        Mat2x3 matrix2x3 = Mat2x3(1, 2, 3, 4, 5, 6);
        TE_LOGGER_INFO("{0:.2f}", position);
        TE_LOGGER_INFO("{0:.2f}", matrix);
        TE_LOGGER_INFO("{0:.2f}", matrix2x3);

        return 0;
    }
}
