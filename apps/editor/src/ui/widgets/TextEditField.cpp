#include <ui/widgets/TextEditField.hpp>

#include <imgui.h>

#include <cstddef>

namespace TechEngine {
    static int resizeTextBuffer(ImGuiInputTextCallbackData* data) {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            std::string* buffer = static_cast<std::string*>(data->UserData);
            buffer->resize(static_cast<std::size_t>(data->BufTextLen));
            data->Buf = buffer->data();
        }
        return 0;
    }

    void TextEditField::requestFocus() {
        m_focusRequested = true;
        m_activated = false;
    }

    bool TextEditField::isEditing() const {
        return m_activated;
    }

    bool TextEditField::draw(const char* label, const std::string& text, std::string& out) {
        if (!m_activated) {
            m_buffer = text;
        }
        if (m_focusRequested) {
            ImGui::SetKeyboardFocusHere();
            m_focusRequested = false;
        }

        const ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackResize;
        const bool entered = ImGui::InputText(label, m_buffer.data(), m_buffer.capacity() + 1, flags, resizeTextBuffer, &m_buffer);
        const bool active = ImGui::IsItemActive();
        m_activated = m_activated || active;
        // A focus request activates the field on a later frame, so an edit ends only once the field was active and no longer is.
        if (!entered && (!m_activated || active)) {
            return false;
        }

        out = m_buffer.c_str();
        m_activated = false;
        return true;
    }
}
