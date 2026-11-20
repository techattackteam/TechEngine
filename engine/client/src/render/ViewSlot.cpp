#include <render/ViewSlot.hpp>

namespace TechEngine {
    void ViewSlot::write(const std::span<const ViewRequest> requests) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_requests.assign(requests.begin(), requests.end());
        m_written = true;
    }

    bool ViewSlot::read(std::vector<ViewRequest>& out) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_written) {
            return false;
        }
        out.assign(m_requests.begin(), m_requests.end());
        return true;
    }

    void ViewSlot::reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_requests.clear();
        m_written = false;
    }
}
