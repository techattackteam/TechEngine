#include <render/UploadChannel.hpp>

#include <iterator>

namespace TechEngine {
    void UploadChannel::send(std::vector<ResourceUploadRecord>& records) {
        const std::lock_guard lock{m_recordMutex};
        m_records.insert(m_records.end(), std::make_move_iterator(records.begin()), std::make_move_iterator(records.end()));
        records.clear();
    }

    void UploadChannel::receive(std::vector<ResourceUploadRecord>& out) {
        const std::lock_guard lock{m_recordMutex};
        out.clear();
        out.swap(m_records);
    }

    void UploadChannel::reportFinished(std::span<const FinishedUpload> uploads) {
        const std::lock_guard lock{m_finishedMutex};
        m_finished.insert(m_finished.end(), uploads.begin(), uploads.end());
    }

    void UploadChannel::takeFinished(std::vector<FinishedUpload>& out) {
        const std::lock_guard lock{m_finishedMutex};
        out.clear();
        out.swap(m_finished);
    }

    void UploadChannel::reset() {
        const std::lock_guard lock{m_recordMutex};
        m_records.clear();
        const std::lock_guard lock2{m_finishedMutex};
        m_finished.clear();
    }
}
