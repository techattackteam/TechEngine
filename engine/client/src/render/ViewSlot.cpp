#include <render/ViewSlot.hpp>

namespace TechEngine {
    void ViewSlot::write(const std::span<const ViewRequest> requests) {
        const std::lock_guard lock{m_mutex};
        m_requests.assign(requests.begin(), requests.end());
        m_written = true;
    }

    bool ViewSlot::read(std::vector<ViewRequest>& out) const {
        const std::lock_guard lock{m_mutex};
        if (!m_written) {
            return false;
        }
        out.assign(m_requests.begin(), m_requests.end());
        return true;
    }

    void ViewSlot::reset() {
        const std::lock_guard lock{m_mutex};
        m_requests.clear();
        m_written = false;
    }
}
