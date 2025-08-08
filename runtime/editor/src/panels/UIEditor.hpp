#pragma once

#include "DockPanel.hpp"
#include "UIHierarchyPanel.hpp"
#include "UIView.hpp"
#include "UIInspector.hpp"
#include "ui/WidgetsRegistry.hpp"

#include <RmlUi/Core/Element.h>

namespace TechEngine {
    class GameView;

    class UIEditor : public DockPanel {
    private:
        UIView m_uiView;
        UIHierarchyPanel m_uiHierarchy;
        UIInspector m_uiInspector;
        WidgetsRegistry m_widgetsRegistry;
        GameView& m_gameView;

        SystemsRegistry& m_appSystemsRegistry;
        Rml::Context* m_context;

        Rml::ElementDocument* m_document;

    public:
        std::shared_ptr<Widget> m_selectedWidget = nullptr;
        std::unordered_map<Rml::Element*, std::shared_ptr<Widget>> m_elementToWidgetMap;

        UIEditor(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry, GameView& sceneView);

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

        GameView& getGameView() const {
            return m_gameView;
        }

    protected:
        void setupInitialDockingLayout() override;

    private:
        void loadRmlDocument(const std::string& path);
    };
}
