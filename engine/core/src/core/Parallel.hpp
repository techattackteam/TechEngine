#pragma once

#include <algorithm>
#include <functional>
#include <thread>
#include <vector>

namespace TechEngine {
    // Runs fn(i) for every i in [0, count) spread across std::thread::hardware_concurrency()
    // chunks, then joins. Mirrors the chunk-and-join pattern used by Query::parallelEachImpl.
    //
    // For small workloads (or single-core machines) it falls back to a plain serial loop to
    // avoid the overhead of spawning threads. `fn` must be safe to call concurrently from
    // multiple threads for disjoint indices.
    inline void parallelFor(size_t count, const std::function<void(size_t)>& fn, size_t serialThreshold = 64) {
        if (count == 0) {
            return;
        }

        const unsigned int hardwareThreads = std::thread::hardware_concurrency();
        if (count <= serialThreshold || hardwareThreads <= 1) {
            for (size_t i = 0; i < count; ++i) {
                fn(i);
            }
            return;
        }

        const unsigned int numThreads = std::min<unsigned int>(hardwareThreads, static_cast<unsigned int>(count));
        std::vector<std::thread> threads;
        threads.reserve(numThreads);

        const size_t chunkSize = (count + numThreads - 1) / numThreads;
        for (unsigned int t = 0; t < numThreads; ++t) {
            const size_t start = static_cast<size_t>(t) * chunkSize;
            const size_t end = std::min(start + chunkSize, count);
            if (start >= end) {
                continue;
            }
            threads.emplace_back([&fn, start, end]() {
                for (size_t i = start; i < end; ++i) {
                    fn(i);
                }
            });
        }

        for (std::thread& thread: threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
}
