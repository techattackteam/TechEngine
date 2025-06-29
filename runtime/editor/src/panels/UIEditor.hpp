#pragma once

#include "DockPanel.hpp"
#include "UIHierarchyPanel.hpp"
#include "UIView.hpp"
#include "UIInspector.hpp"
#include "renderer/ui/widget/Widget.hpp"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Factory.h>

#include "core/Logger.hpp"
#include "renderer/ui/widget/Canvas.hpp"

namespace TechEngine {
    class UIEditor : public DockPanel {
    private:
        UIView m_uiView;
        UIHierarchyPanel m_uiHierarchy;
        UIInspector m_uiInspector;

        SystemsRegistry& m_appSystemsRegistry;
        Rml::Context* m_context;

        Rml::ElementDocument* m_document;
        Widget* m_selectedWidget = nullptr;
        std::unordered_map<Rml::Element*, Widget*> m_elementToWidgetMap;
        std::vector<std::unique_ptr<Widget>> m_topLevelWidgets;

    public:
        UIEditor(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry);

        void onInit() override;

        void onUpdate() override;

        template<typename T, typename... Args>
        void createWidget(Rml::Element* parent, Args&&... args) {
            T* newWidget = new T(std::forward<Args>(args)...);

            Rml::ElementPtr newRmlElement = Rml::Factory::InstanceElement(nullptr, "div", "div", Rml::XMLAttributes()); //Not sure if this is the right way to create an element
            if (!newRmlElement) {
                Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to create Rml::Element for widget: %s", newWidget->getRmlTag());
                return;
            }
            Rml::Element* element;
            if (parent != nullptr) {
                element = parent->AppendChild(std::move(newRmlElement));
            } else {
                element = m_document->AppendChild(std::move(newRmlElement));
            }

            newWidget->setRmlElement(element);

            m_elementToWidgetMap[element] = newWidget;
            std::vector<WidgetProperty> properties = newWidget->getProperties();
            for (auto& prop: properties) {
                prop.onChange(prop.value); // Initialize properties with their initial values
            }
            element->SetProperty("position", "absolute");

            m_context->Update();
            TE_LOGGER_INFO("Element: {0} created with tag: {1}", newWidget->getName().c_str(), newWidget->getRmlTag());
            TE_LOGGER_INFO("Element Top: {0}", element->GetAbsoluteTop());
        }

        void setSelectedWidget(Widget* widget);

        void drawRmlElementInHierarchy(Rml::Element* element);

    protected:
        void setupInitialDockingLayout() override;

    private:
        void loadRmlDocument(const std::string& path);


        void buildWidgetMapForDocument(Rml::Element* root_element);
    };
}
