#pragma once

#include "HierarchyNode.hpp"
#include "HierarchyPanelBase.hpp"
#include "TechEngine/core/components/Entity.hpp"
#include "TechEngine/core/scene/Scene.hpp"

namespace TechEngine {
    class ResourceSystem;
    class EventManager;

    class EntityHierarchyPanel : public HierarchyPanelBase<Entity> {
    public:
        enum class EntityType {
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
            Entity parent = static_cast<Entity>(-1);
        };

    private:
        SystemsRegistry& m_appSystemsRegistry;

        HierarchyNode& m_legacySelectedNode;

        std::vector<EntityToCreate> m_entitiesToCreate;
        std::vector<Entity> m_entitiesToDelete;
        std::vector<Entity> m_entitiesToDuplicate;

    public:
        EntityHierarchyPanel(
            SystemsRegistry& editorSystemsRegistry,
            SystemsRegistry& appSystemsRegistry,
            HierarchyNode& selectedNode
        );

        ~EntityHierarchyPanel() override = default;

    protected:
        Entity getInvalidNodeId() const override {
            return static_cast<Entity>(-1);
        }

        bool isValidNodeId(Entity id) const override {
            return id != static_cast<Entity>(-1);
        }

        void rebuildCache() override;

        std::string getNodeName(Entity id) const override;

        Entity getNodeParent(Entity id) const override;

        std::vector<Entity> getNodeChildren(Entity id) const override;

        void onSelectionChanged(Entity newSelection) override;

        void onHierarchyInit() override;

        void drawEmptySpaceContextMenu() override;

        void drawNodeContextMenu(Entity entity) override;

        void reparentNode(Entity node, Entity newParent, Entity insertBefore) override;

        void processQueuedOperations() override;

    private:
        // Helper methods
        void drawCreateEntityMenu(const std::string& title, Entity parent);

        void createEntity(const EntityToCreate& entityInfo);

        void deleteEntity(Entity entity);

        Scene& getActiveScene() const;
    };
}
