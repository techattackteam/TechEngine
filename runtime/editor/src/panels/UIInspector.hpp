#pragma once
#include "Panel.hpp"
#include "ui/Widget.hpp"

#include <imgui_stdlib.h>

namespace TechEngine {
    class UIInspector : public Panel {
    private:
        std::shared_ptr<Widget> m_selectedWidget = nullptr;
        SystemsRegistry& m_appSystemsRegistry;

    public:
        explicit UIInspector(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry);

        void onInit() override;

        void onUpdate() override;

        void setSelectedWidget(std::shared_ptr<Widget> widget);

    private:
        template<typename T>
        void inspectTextWidget(T* widget) {
            std::string text = widget->getText();

            if (resizableInputTextMultiline("##Label", &text, ImVec2(-1.0f, ImGui::GetTextLineHeight() * 4), ImGui::GetTextLineHeight() * 4)) {
                widget->setText(text);
            }

            ImGui::ColorEdit4("Color", &widget->getColor().x);
            // Font Selection (assuming you have a FontManager)
            // For now, let's use a placeholder.
            // To implement this fully, your FontManager should provide a vector of font names.
            const char* fonts[] = {"Arial", "Roboto", "Times New Roman"};
            static int currentFont = 0;
            if (ImGui::Combo("Font", &currentFont, fonts, IM_ARRAYSIZE(fonts))) {
                // Example: setFontPath(fontManager->getPathFor(fonts[currentFont]));
            }

            // Font Size
            if (ImGui::DragFloat("Font Size", &widget->getFontSize(), 0.5f, 8.0f, 72.0f)) {
                // Value is updated directly
            }

            // Font Style
            ImGui::Checkbox("Bold", &widget->isBold());
            ImGui::SameLine();
            ImGui::Checkbox("Italic", &widget->isItalic());

            // Horizontal Alignment
            const char* h_align_items[] = {"Left", "Center", "Right"};
            int current_h_align = static_cast<int>(widget->getHorizontalAlign());
            if (ImGui::Combo("H-Align", &current_h_align, h_align_items, IM_ARRAYSIZE(h_align_items))) {
                //m_hAlign = static_cast<HorizontalAlignment>(current_h_align);
            }

            // Vertical Alignment
            const char* v_align_items[] = {"Top", "Middle", "Bottom"};
            int current_v_align = static_cast<int>(widget->getVerticalAlign());
            if (ImGui::Combo("V-Align", &current_v_align, v_align_items, IM_ARRAYSIZE(v_align_items))) {
                //m_vAlign = static_cast<VerticalAlignment>(current_v_align);
            }
        }

        bool resizableInputTextMultiline(const char* label, std::string* text, const ImVec2& initial_size, float min_height);
    };
}
