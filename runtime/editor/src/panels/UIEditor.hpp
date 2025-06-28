#pragma once

#include "DockPanel.hpp"
#include "UIHierarchyPanel.hpp"
#include "UIView.hpp"

#include <RmlUi/Core/Element.h>

#include "UIInspector.hpp"

namespace TechEngine {
    class UIEditor : public DockPanel {
    private:
        UIView m_uiView;
        UIHierarchyPanel m_uiHierarchy;
        UIInspector m_uiInspector;
        
        
        SystemsRegistry& m_appSystemsRegistry;
        Rml::Context* m_context;


        std::string m_currentRmlDocumentPath;

        // Element selection
        Rml::Element* m_selectedElement = nullptr;
        ImVec2 m_lastMousePosInImage = ImVec2(0, 0); // Store mouse position within the image

        // Dragging state
        bool m_isDragging = false;
        Rml::Element* m_draggedElement = nullptr;
        ImVec2 m_dragStartMousePosInImage; // Mouse position within the FBO image when drag started
        Rml::Vector2f m_dragStartElementOffsets; // Original 'left' and 'top' style properties of the element
    public:
        UIEditor(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry);

        void onInit() override;


        void onUpdate() override;

    protected:
        void setupInitialDockingLayout() override;

    private:
        void drawElementInHierarchy(Rml::Element* element);

        void loadRmlDocument(const std::string& path);
    };
}
