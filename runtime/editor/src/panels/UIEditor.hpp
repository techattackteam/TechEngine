#pragma once

#include "DockPanel.hpp"
#include "UIHierarchyPanel.hpp"
#include "UIView.hpp"
#include "UIInspector.hpp"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Factory.h>

#include "core/Logger.hpp"
#include "ui/WidgetsRegistry.hpp"

namespace TechEngine {
    class UIEditor : public DockPanel {
    private:
        UIView m_uiView;
        UIHierarchyPanel m_uiHierarchy;
        UIInspector m_uiInspector;
        WidgetsRegistry m_widgetsRegistry;

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

        void createWidget(Rml::Element* parent, const std::string& name) {
            Widget* newWidget = m_widgetsRegistry.createWidget(name); // Assuming this function creates a widget and returns a pointer to it

            Rml::ElementPtr newRmlElement = Rml::Factory::InstanceElement(nullptr, "div", "div", Rml::XMLAttributes()); //Not sure if this is the right way to create an element
            if (!newRmlElement) {
                //Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to create Rml::Element for widget: %s", newWidget->getRmlTag());
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
                if (prop.type == "int") {
                    prop.onChange(std::stoi(prop.defaultValue)); // Convert string to int
                } else if (prop.type == "float") {
                    prop.onChange(std::stof(prop.defaultValue));
                } else if (prop.type == "bool") {
                    prop.onChange(prop.defaultValue == "true"); // Convert string to bool
                } else if (prop.type == "string") {
                    prop.onChange(prop.defaultValue); // Keep as string
                } else if (prop.type == "vector2f") {
                    Rml::Vector2f vec;
                    sscanf_s(prop.defaultValue.c_str(), "%f,%f", &vec.x, &vec.y);
                    prop.onChange(vec);
                } else if (prop.type == "vector3f") {
                    Rml::Vector3f vec;
                    sscanf_s(prop.defaultValue.c_str(), "%f,%f,%f", &vec.x, &vec.y, &vec.z);
                    prop.onChange(vec);
                } else if (prop.type == "color") {
                    Rml::Colourb color = Rml::Colourb(255, 255, 255, 255); // Default to white
                    //sscanf_s(prop.defaultValue.c_str(), "rgba(%hhu,%hhu,%hhu,%hhu)", &color.red, &color.green, &color.blue, &color.alpha);
                    prop.onChange(color);
                }
                prop.onChange(prop.defaultValue); // Initialize properties with their initial values
            }
            element->SetProperty("position", "absolute");
            element->SetProperty("background-color", "#3498db");
            element->SetProperty("border", "2px solid white");
            element->SetProperty("align-self", "center");
            element->SetProperty("transform-origin-x", "center");
            element->SetProperty("transform-origin-y", "center");

            //Set background color to blue
            m_context->Update();
            //TE_LOGGER_INFO("Element: {0} created with tag: {1}", newWidget->getName().c_str(), newWidget->getRmlTag());
            TE_LOGGER_INFO("Element Top: {0}", element->GetAbsoluteTop());
        }

        void setSelectedWidget(Widget* widget);

        Widget* getSelectedWidget() const {
            return m_selectedWidget;
        }

        void drawRmlElementInHierarchy(Rml::Element* element);

        const std::unordered_map<Rml::Element*, Widget*>& getElementToWidgetMap();

        const WidgetsRegistry& getWidgetsRegistry() const {
            return m_widgetsRegistry;
        }

    protected:
        void setupInitialDockingLayout() override;

    private:
        void loadRmlDocument(const std::string& path);


        void buildWidgetMapForDocument(Rml::Element* root_element);
    };
}
