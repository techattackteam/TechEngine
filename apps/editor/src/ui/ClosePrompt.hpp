#pragma once

#include <cstdint>

namespace TechEngine {
    enum class CloseChoice : std::uint8_t {
        None,
        Save,
        Discard,
        Cancel,
    };

    class ClosePrompt {
    private:
        bool m_openRequested = false;

    public:
        void open();

        // ImGui opens a modal only from inside a frame, so open() is honoured by the next draw().
        CloseChoice draw();
    };
}
