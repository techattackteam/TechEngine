#pragma once

#include <TechEngine/core/resources/ResourceUploadRecord.hpp>

#include <mutex>
#include <span>
#include <vector>

namespace TechEngine {
    class UploadChannel {
    private:
        std::mutex m_recordMutex;
        std::vector<ResourceUploadRecord> m_records;
        std::mutex m_finishedMutex;
        std::vector<FinishedUpload> m_finished;

    public:
        UploadChannel() = default;

        UploadChannel(const UploadChannel&) = delete;

        UploadChannel& operator=(const UploadChannel&) = delete;

        UploadChannel(UploadChannel&&) = delete;

        UploadChannel& operator=(UploadChannel&&) = delete;

        // Moves every record out and leaves `records` empty, with its capacity kept for the next publish.
        void send(std::vector<ResourceUploadRecord>& records);

        void receive(std::vector<ResourceUploadRecord>& out);

        void reportFinished(std::span<const FinishedUpload> uploads);

        void takeFinished(std::vector<FinishedUpload>& out);

        void reset();
    };
}
