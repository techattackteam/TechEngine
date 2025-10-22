#pragma once


#include <functional>

#include "Panel.hpp"
#include "TechEngine/core/components/Entity.hpp"

#include <memory>

#include "HierarchyNode.hpp"
#include "ui/Widget.hpp"

namespace TechEngine {
    enum class ProjectType;
    class UIEditor;
    class Widget;

    class SceneSettingsPanel : public Panel {
    private:
        enum EntityType {
            Empty,
            Cube,
            Sphere,
            Cylinder,
            Capsule,
            Camera,
            Light
        };

        struct EntityToCreate {
            std::string name;
            EntityType type;
            Entity parent = (uint32_t)-1; // Default to no parent
        };


        SystemsRegistry& m_appSystemsRegistry;
        ProjectType& m_projectType;

        bool m_isItemHovered = false;

        std::vector<HierarchyNode> m_displayEntitiesList;
        std::vector<HierarchyNode> m_displayWidgetList;
        HierarchyNode& m_selectedNode;
        bool m_isHierarchyDirty = true;
        bool m_isNodeHovered = false;

        std::vector<EntityToCreate> m_nodesToCreate;
        std::vector<HierarchyNode> m_nodesToDelete;

    public:
        SceneSettingsPanel(ProjectType& projectType, SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry, HierarchyNode& selectedNode);

        void onInit() override;

        void onUpdate() override;

        void drawTree(HierarchyNode::NodeType nodeType);

    private:
        bool drawStyledMainTreeNode(const char* label);

        void drawRendererTree();

        void drawNode(HierarchyNode& node);

        void drawDropZone(const std::shared_ptr<Widget>& parent, const std::shared_ptr<Widget>& targetBefore);

        void rebuildDisplayList(HierarchyNode::NodeType nodeType);

        void recursiveAddToDisplayList(const std::shared_ptr<Widget>& widget, int depth);

        void openRightClickMenu(HierarchyNode& clickedNode);

        void openEntityMenu(HierarchyNode& clickedNode);

        void openRightClickMenu(HierarchyNode::NodeType nodeType);

        void openCreateEntityMenu(std::string title, Entity parent);

        void requestEntitiesCreation();

        void setHierarchyDirty();
    };
}
