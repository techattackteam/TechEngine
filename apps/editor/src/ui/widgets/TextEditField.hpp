#pragma once

#include <string>

namespace TechEngine {
    class TextEditField {
    private:
        std::string m_buffer;
        bool m_focusRequested = false;
        bool m_activated = false;

    public:
        void requestFocus();

        bool isEditing() const;

        // Shows text until the user activates the field. Returns true once, on the frame the edit
        // ends, with the final text in out; Escape ends it with the text it started from.
        bool draw(const char* label, const std::string& text, std::string& out);
    };
}
