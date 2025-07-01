#include "UIEditor.hpp"

#include <imgui_internal.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include "renderer/FrameBuffer.hpp"
#include "renderer/Renderer.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    UIEditor::UIEditor(SystemsRegistry& editorSystemsRegistry,
                       SystemsRegistry& appSystemsRegistry) : DockPanel(editorSystemsRegistry),
                                                              m_uiView(editorSystemsRegistry, appSystemsRegistry, this),
                                                              m_uiHierarchy(editorSystemsRegistry, appSystemsRegistry),
                                                              m_uiInspector(editorSystemsRegistry),
                                                              m_appSystemsRegistry(appSystemsRegistry),
                                                              m_context(nullptr) {
        m_styleVars.emplace_back(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    }

    void UIEditor::onInit() {
        m_context = m_appSystemsRegistry.getSystem<Renderer>().getUIContext();
        if (m_context) {
            //loadRmlDocument("C:\\dev\\TechEngine\\bin\\runtime\\editor\\debug\\New Project\\resources\\client\\assets\\ui\\sample.rml"); // m_filePathBuffer defaults to "assets/ui/sample.rml"
            Rml::ElementDocument* doc = m_context->GetDocument(0);
            if (!doc) {
                TE_LOGGER_ERROR("UIEditor: No RmlUi document found in the context.");
                return;
            }
        }
        m_document = m_context->GetDocument(0); // For now lets assume the first document is the one we want to edit
        m_uiHierarchy.setEditor(this);
        m_uiHierarchy.init("UI Hierarchy", &m_dockSpaceWindowClass);
        m_uiView.init("UI View", &m_dockSpaceWindowClass);
        m_uiInspector.init("UI Inspector", &m_dockSpaceWindowClass);
        m_widgetsRegistry.load("C:\\dev\\TechEngine\\bin\\runtime\\editor\\debug\\New Project\\resources\\client\\assets\\ui\\widgets.json");
        //createWidget<Canvas>(nullptr, "Panel"); // Create a default panel to start with
    }

    void UIEditor::onUpdate() {
        m_uiHierarchy.update();
        m_uiInspector.update();
        m_uiView.update();
    }

    void UIEditor::setSelectedWidget(Widget* widget) {
        m_selectedWidget = widget;
        m_uiInspector.setSelectedWidget(widget);
    }

    void UIEditor::setupInitialDockingLayout() {
        ImGui::DockBuilderRemoveNode(m_dockSpaceID);
        ImGui::DockBuilderAddNode(m_dockSpaceID, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(m_dockSpaceID, ImGui::GetMainViewport()->Size);

        ImGuiID dockMainID = m_dockSpaceID;
        ImGuiID dockRightID = ImGui::DockBuilderSplitNode(dockMainID, ImGuiDir_Right, 0.2f, nullptr, &dockMainID);
        ImGuiID dockLeftID = ImGui::DockBuilderSplitNode(dockMainID, ImGuiDir_Left, 0.2f, nullptr, &dockMainID);

        ImGui::DockBuilderDockWindow((m_uiView.getName() + "##" + std::to_string(m_uiView.getId())).c_str(),
                                     dockMainID);
        ImGui::DockBuilderDockWindow(
            (m_uiHierarchy.getName() + "##" + std::to_string(m_uiHierarchy.getId())).c_str(),
            dockLeftID);
        ImGui::DockBuilderDockWindow(
            (m_uiInspector.getName() + "##" + std::to_string(m_uiInspector.getId())).c_str(), dockRightID);

        ImGui::DockBuilderFinish(m_dockSpaceID);
    }


    void UIEditor::loadRmlDocument(const std::string& path) {
        if (!m_context) {
            TE_LOGGER_ERROR("Cannot load RML document: UI Context is null.");
            return;
        }
        //m_context->UnloadAllDocuments(); // Unload previous document if any
        //Rml::ElementDocument* doc = m_context->LoadDocument(path);
        //if (doc) {
        //    doc->Show();
        //    // doc->RemoveReference(); // Context owns it
        //    m_currentRmlDocumentPath = path;
        //    TE_LOGGER_INFO("InteractiveUIEditorPanel: Loaded RML document '%s' into editor context.", path.c_str());
        //} else {
        //    TE_LOGGER_ERROR("InteractiveUIEditorPanel: Failed to load RML document '%s'.", path.c_str());
        //    m_currentRmlDocumentPath.clear();
        //}
    }

    void UIEditor::drawRmlElementInHierarchy(Rml::Element* element) {
        if (!element) return;

        // Find our C++ widget counterpart for this element
        Widget* widget = nullptr;
        auto it = m_elementToWidgetMap.find(element);
        if (it != m_elementToWidgetMap.end()) {
            widget = it->second;
        }

        // Determine the label for the ImGui node
        std::string label = element->GetTagName();
        if (!element->GetId().empty()) {
            label += "#" + element->GetId();
        }
        if (widget) {
            // If we have a C++ widget, use its name for a friendlier display
            label = widget->getName() + " (" + label + ")";
        }

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
        if (m_selectedWidget == widget) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        // Make leaf nodes (like text) not have an arrow
        if (element->GetNumChildren() == 0) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        bool node_open = ImGui::TreeNodeEx((void*)element, flags, "%s", label.c_str());

        if (ImGui::IsItemClicked() && widget) {
            setSelectedWidget(widget);
        }

        // Add a right-click menu to create children
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Add Panel")) {
                //createWidget<Canvas>(element, "Panel"); // Create a child for the currently right-clicked Rml::Element
            }
            ImGui::EndPopup();
        }


        if (node_open) {
            for (int i = 0; i < element->GetNumChildren(); ++i) {
                drawRmlElementInHierarchy(element->GetChild(i));
            }
            ImGui::TreePop();
        }
    }

    const std::unordered_map<Rml::Element*, Widget*>& UIEditor::getElementToWidgetMap() {
        return m_elementToWidgetMap;
    }

    void UIEditor::buildWidgetMapForDocument(Rml::Element* root_element) {
        // Clear old data
        m_topLevelWidgets.clear();
        m_elementToWidgetMap.clear();

        // We can't create a C++ widget for every single Rml::Element (like text nodes),
        // so for now, we only care about the ones we create explicitly.
        // The hierarchy will still draw everything from the Rml DOM.
        // Let's create a C++ widget for the body to act as our logical root.

        //Rml::Element* body = root_element->QuerySelector("body");
        //if (body) {
        //    auto canvasWidget = std::make_unique<Canvas>("Body");
        //    canvasWidget->setRmlElement(body);
        //    m_elementToWidgetMap[body] = canvasWidget.get();
        //    m_topLevelWidgets.push_back(std::move(canvasWidget));
        //}
    }
}
