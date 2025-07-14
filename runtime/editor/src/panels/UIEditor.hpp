#pragma once

#include "DockPanel.hpp"
#include "UIHierarchyPanel.hpp"
#include "UIView.hpp"
#include "UIInspector.hpp"

#include <RmlUi/Core/Element.h>
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

    public:
        std::shared_ptr<Widget> m_selectedWidget = nullptr;
        std::unordered_map<Rml::Element*, std::shared_ptr<Widget>> m_elementToWidgetMap;

        UIEditor(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry);

        void onInit() override;

        void onUpdate() override;

        std::shared_ptr<Widget> createWidget(Rml::Element* parent, const std::string& name, bool base);

        bool deleteWidget(const std::shared_ptr<Widget>& widget);

        void setSelectedWidget(const std::shared_ptr<Widget>& widget);

        std::shared_ptr<Widget> getSelectedWidget() const {
            return m_selectedWidget;
        }

        const std::unordered_map<Rml::Element*, std::shared_ptr<Widget>>& getElementToWidgetMap();

        const WidgetsRegistry& getWidgetsRegistry() const {
            return m_widgetsRegistry;
        }

    protected:
        void setupInitialDockingLayout() override;

    private:
        void loadRmlDocument(const std::string& path);
    };
}
