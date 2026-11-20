#pragma once

#include <TechEngine/client/render/RenderView.hpp>

#include <mutex>
#include <span>
#include <vector>

namespace TechEngine {
    class ViewSlot {
    private:
        mutable std::mutex m_mutex;
        std::vector<ViewRequest> m_requests;
        bool m_written = false;

    public:
        ViewSlot() = default;

        ViewSlot(const ViewSlot&) = delete;

        ViewSlot& operator=(const ViewSlot&) = delete;

        ViewSlot(ViewSlot&&) = delete;

        ViewSlot& operator=(ViewSlot&&) = delete;

        void write(std::span<const ViewRequest> requests);

        // False until the first write. A read never consumes, so render reads the same views until main writes again.
        bool read(std::vector<ViewRequest>& out) const;

        void reset();
    };
}
