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
                                                              m_uiView(editorSystemsRegistry, appSystemsRegistry),
                                                              m_uiHierarchy(editorSystemsRegistry, appSystemsRegistry),
                                                              m_uiInspector(editorSystemsRegistry),
                                                              m_appSystemsRegistry(appSystemsRegistry),
                                                              m_context(nullptr) {
        m_styleVars.emplace_back(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    }

    void UIEditor::onInit() {
        m_context = m_appSystemsRegistry.getSystem<Renderer>().getUIContext();
        m_uiHierarchy.init("UI Hierarchy", &m_dockSpaceWindowClass);
        m_uiView.init("UI View", &m_dockSpaceWindowClass);
        m_uiInspector.init("UI Inspector", &m_dockSpaceWindowClass);

        if (m_context) {
            //loadRmlDocument("C:\\dev\\TechEngine\\bin\\runtime\\editor\\debug\\New Project\\resources\\client\\assets\\ui\\sample.rml"); // m_filePathBuffer defaults to "assets/ui/sample.rml"
        }
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

    void UIEditor::onUpdate() {
        m_uiHierarchy.update();
        m_uiInspector.update();
        m_uiView.update();
        /*// Layout: Hierarchy | RMLView | Inspector
        float hierarchyWidth = 200.0f;
        float inspectorWidth = 250.0f;
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float rmlViewWidth = availableWidth - hierarchyWidth - inspectorWidth - ImGui::GetStyle().ItemSpacing.x * 2;
        if (rmlViewWidth < 100) {
            rmlViewWidth = 100;
        }
        FrameBuffer& frameBuffer = m_appSystemsRegistry.getSystem<Renderer>().getFramebuffer(m_frameBufferID);
        frameBuffer.bind();
        // --- Hierarchy Panel ---
        ImGui::BeginChild("Hierarchy", ImVec2(hierarchyWidth, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
        if (m_context) {
            if (m_context->GetNumDocuments() > 0) {
                for (int i = 0; i < m_context->GetNumDocuments(); ++i) {
                    Rml::ElementDocument* doc = m_context->GetDocument(i);
                    if (doc) {
                        ImGuiTreeNodeFlags doc_node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
                        bool doc_node_open = ImGui::TreeNodeEx((void*)doc, doc_node_flags, "Document: %s", doc->GetSourceURL().c_str());
                        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                            /* Select doc? #1#
                        }
                        if (doc_node_open) {
                            /*if (doc->GetBody()) drawElementInHierarchy(doc->GetBody());
                            else {
                                for (int child_idx = 0; child_idx < doc->GetNumChildren(); ++child_idx) {
                                    drawElementInHierarchy(doc->GetChild(child_idx));
                                }
                            } #1#
                            ImGui::TreePop();
                        }
                    }
                }
            } else {
                ImGui::Text("No documents in context.");
            }
        } else {
            ImGui::Text("UI Context not initialized.");
        }
        ImGui::EndChild(); // End Hierarchy
        ImGui::SameLine();
        // Center Pane: RML View (FBO)
        ImGui::BeginChild("RMLViewPane", ImVec2(0, 0), false); // Leave space at bottom if needed, or ImVec2(0,0)
        ImVec2 rmlViewSize = ImGui::GetContentRegionAvail();
        if (rmlViewSize.x <= 0) rmlViewSize.x = 1;
        if (rmlViewSize.y <= 0) rmlViewSize.y = 1;
        // Resize FBO if RML View size changed
        if (m_frameBufferID && (static_cast<int>(rmlViewSize.x) != frameBuffer.width || static_cast<int>(rmlViewSize.y) != frameBuffer.height)) {
            if (rmlViewSize.x > 0 && rmlViewSize.y > 0) {
                frameBuffer.resize(rmlViewSize.x, rmlViewSize.y);
                m_context->SetDimensions(Rml::Vector2i(frameBuffer.width, frameBuffer.height));
            }
        }
        UIRenderer& uiRenderer = m_appSystemsRegistry.getSystem<Renderer>().getUIRenderer();
        uiRenderer.onUpdate();
        ImGui::Image((void*)(intptr_t)frameBuffer.colorTexture, ImVec2((float)frameBuffer.width, (float)frameBuffer.height), ImVec2(0, 1), ImVec2(1, 0));

        ImGui::EndChild();
        frameBuffer.unBind();
        if (ImGui::IsItemHovered()) {
            ImVec2 mousePosAbsolute = ImGui::GetMousePos();
            ImVec2 imagePosAbsolute = ImGui::GetItemRectMin();
            m_lastMousePosInImage = ImVec2(mousePosAbsolute.x - imagePosAbsolute.x, mousePosAbsolute.y - imagePosAbsolute.y);

            // Forward mouse move to the RmlUi context for its own hover effects etc.
            ImGuiIO& io = ImGui::GetIO();
            int rml_mods = 0;
            if (io.KeyCtrl) rml_mods |= Rml::Input::KM_CTRL;
            if (io.KeyShift) rml_mods |= Rml::Input::KM_SHIFT;
            if (io.KeyAlt) rml_mods |= Rml::Input::KM_ALT;
            // Add KM_META for Super/Command if RmlUi supports it and ImGui provides it (io.KeySuper)

            m_context->ProcessMouseMove((int)m_lastMousePosInImage.x, (int)m_lastMousePosInImage.y, rml_mods);

            // --- Dragging Logic ---
            if (m_isDragging && m_draggedElement) {
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    Rml::Vector2f mouseDelta(m_lastMousePosInImage.x - m_dragStartMousePosInImage.x, m_lastMousePosInImage.y - m_dragStartMousePosInImage.y);
                    Rml::Vector2f newPos(m_dragStartElementOffsets.x + mouseDelta.x, m_dragStartElementOffsets.y + mouseDelta.y);

                    // Ensure the element has a style that allows 'left' and 'top' to be effective, e.g., position: relative or absolute
                    // This might require setting it if not already set:
                    // if (m_draggedElement->GetProperty<Rml::String>("position") == "static" || m_draggedElement->GetProperty<Rml::String>("position").empty()){
                    //    m_draggedElement->SetProperty("position", Rml::Property("relative", Rml::Property::KEYWORD));
                    // }

                    /*m_draggedElement->SetProperty("left", Rml::Property(newPos.x, Rml::Property::PX));
                    m_draggedElement->SetProperty("top", Rml::Property(newPos.y, Rml::Property::PX));
                    m_draggedElement->DirtyProperties();#1#
                } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    m_isDragging = false;
                    m_draggedElement = nullptr;
                    TE_LOGGER_INFO("Editor: Drag ended.");
                }
            }

            // --- Mouse Click/Release for Selection and Context Interaction (modified to incorporate drag start) ---
            for (int i = 0; i < ImGuiMouseButton_COUNT; ++i) {
                if (ImGui::IsMouseClicked(i)) {
                    m_context->ProcessMouseButtonDown(i, rml_mods); // Let RmlUI context process first

                    if (i == ImGuiMouseButton_Left && !m_isDragging) { // Start drag only if not already dragging
                        Rml::Element* element_under_mouse = m_context->GetElementAtPoint(Rml::Vector2f(m_lastMousePosInImage.x, m_lastMousePosInImage.y));

                        // Update selection
                        if (m_selectedElement != element_under_mouse) {
                            if (m_selectedElement) m_selectedElement->SetClass("editor-selected", false);
                            m_selectedElement = element_under_mouse;
                            if (m_selectedElement) m_selectedElement->SetClass("editor-selected", true);
                            if (m_selectedElement)
                                TE_LOGGER_INFO("Editor: Selected element %s", m_selectedElement->GetAddress().c_str());
                            else
                                TE_LOGGER_INFO("Editor: Deselected.");
                        }

                        // If an element is selected (could be the one just selected or a pre-selected one), start dragging it
                        if (m_selectedElement && m_selectedElement != m_context->GetRootElement() /*&& m_selectedElement != m_context->GetBody()#1#) { // Don't drag root/body
                            m_isDragging = true;
                            m_draggedElement = m_selectedElement;
                            m_dragStartMousePosInImage = m_lastMousePosInImage;

                            // Get current 'left' and 'top' style properties
                            const Rml::Property* left_prop = m_draggedElement->GetProperty("left");
                            const Rml::Property* top_prop = m_draggedElement->GetProperty("top");
                            m_dragStartElementOffsets.x = left_prop ? left_prop->Get<float>() : 0.0f; // Assumes PX unit or number
                            m_dragStartElementOffsets.y = top_prop ? top_prop->Get<float>() : 0.0f;
                            // More robust: check unit, convert if necessary. For POC, assume PX.
                            TE_LOGGER_INFO("Editor: Drag started on %s. StartOffset: (%.1f, %.1f)", m_draggedElement->GetAddress().c_str(), m_dragStartElementOffsets.x, m_dragStartElementOffsets.y);
                        }
                    }
                }
                if (ImGui::IsMouseReleased(i)) {
                    m_context->ProcessMouseButtonUp(i, rml_mods);
                    if (i == ImGuiMouseButton_Left && m_isDragging) { // Finalize drag on release
                        m_isDragging = false;
                        m_draggedElement = nullptr; // Keep selection, but stop drag state
                        TE_LOGGER_INFO("Editor: Drag ended on release.");
                    }
                }
            }

            float scrollY = io.MouseWheel;
            if (scrollY != 0.0f) {
                m_context->ProcessMouseWheel(-scrollY, rml_mods);
            }
        } else { // Mouse not hovering the RML view
            if (m_isDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                // If mouse was released outside the window while dragging
                m_isDragging = false;
                m_draggedElement = nullptr;
                TE_LOGGER_INFO("Editor: Drag ended (mouse released outside).");
            }
        }*/
    }

    void UIEditor::drawElementInHierarchy(Rml::Element* element) {
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
}
