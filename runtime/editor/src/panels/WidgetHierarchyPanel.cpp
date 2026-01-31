#include "WidgetHierarchyPanel.hpp"

#include "systems/SystemsRegistry.hpp"
#include "ui/WidgetsRegistry.hpp"
#include "UIUtils/ImGuiUtils.hpp"

namespace TechEngine {

    WidgetHierarchyPanel::WidgetHierarchyPanel(
        SystemsRegistry& editorSystemsRegistry,
        SystemsRegistry& appSystemsRegistry,
        HierarchyNode& selectedNode
    ) : HierarchyPanelBase<std::shared_ptr<Widget>>(editorSystemsRegistry, HierarchyPanelConfig{
            .allowReparenting = true,
            .allowReordering = true,
            .allowCreation = true,
            .allowDeletion = true,
            .allowRenaming = true,
            .showDropZones = true,
            .preserveWorldTransform = false  // Widgets don't have world transforms
        }),
        m_appSystemsRegistry(appSystemsRegistry),
        m_legacySelectedNode(selectedNode)
    {
        m_dragDropPayloadName = "WIDGET_DRAG";
        m_nodeCache.reserve(64);
        m_rootNodes.reserve(16);
    }

    void WidgetHierarchyPanel::onHierarchyInit() {
        // Could subscribe to widget creation events here if needed
    }

    void WidgetHierarchyPanel::rebuildCache() {
        m_nodeCache.clear();
        m_rootNodes.clear();
        m_dirtyNodes.clear();

        WidgetsRegistry& registry = getWidgetsRegistry();

        // Build cache from root widgets
        for (const WidgetPtr& widget : registry.getRootWidgets()) {
            if (widget) {
                m_rootNodes.push_back(widget);
                rebuildCacheRecursive(widget, nullptr, 0);
            }
        }

        m_totalNodeCount = m_nodeCache.size();
    }

    void WidgetHierarchyPanel::rebuildCacheRecursive(WidgetPtr widget, WidgetPtr parent, int depth) {
        if (!widget) return;

        NodeCache cache;
        cache.id = widget;
        cache.parent = parent;
        cache.name = widget->m_name;
        cache.depth = depth;
        cache.isLoaded = true;

        // Preserve expansion state
        auto oldIt = m_nodeCache.find(widget);
        if (oldIt != m_nodeCache.end()) {
            cache.isExpanded = oldIt->second.isExpanded;
        }

        // Add children
        for (const auto& child : widget->m_children) {
            if (child) {
                cache.children.push_back(child);
            }
        }

        m_nodeCache[widget] = std::move(cache);

        // Recursively add children
        for (const auto& child : widget->m_children) {
            rebuildCacheRecursive(child, widget, depth + 1);
        }
    }

    std::string WidgetHierarchyPanel::getNodeName(WidgetPtr widget) const {
        if (widget) {
            return widget->m_name;
        }
        return "Unknown Widget";
    }

    WidgetHierarchyPanel::WidgetPtr WidgetHierarchyPanel::getNodeParent(WidgetPtr widget) const {
        if (widget) {
            return widget->m_parent;
        }
        return nullptr;
    }

    std::vector<WidgetHierarchyPanel::WidgetPtr> WidgetHierarchyPanel::getNodeChildren(WidgetPtr widget) const {
        if (widget) {
            return widget->m_children;
        }
        return {};
    }

    void WidgetHierarchyPanel::onSelectionChanged(WidgetPtr newSelection) {
        if (newSelection) {
            auto it = m_nodeCache.find(newSelection);
            if (it != m_nodeCache.end()) {
                HierarchyNode node;
                node.type = HierarchyNode::NodeType::Widget;
                node.widget = newSelection;
                node.id = reinterpret_cast<uintptr_t>(newSelection.get());
                node.name = it->second.name;
                node.depth = it->second.depth;
                node.isOpen = it->second.isExpanded;
                m_legacySelectedNode = node;
            }
        } else {
            m_legacySelectedNode = HierarchyNode();
        }
    }

    void WidgetHierarchyPanel::drawEmptySpaceContextMenu() {
        if (m_config.allowCreation) {
            if (ImGui::BeginMenu("Create Widget")) {
                WidgetsRegistry& registry = getWidgetsRegistry();

                for (const auto& widgetTemplate : registry.getWidgetTemplates()) {
                    if (ImGui::MenuItem(widgetTemplate.name.c_str())) {
                        registry.createWidget(nullptr, widgetTemplate.name, widgetTemplate.type, false);
                        markFullRebuildNeeded();
                    }
                }

                ImGui::EndMenu();
            }
        }
    }

    void WidgetHierarchyPanel::drawNodeContextMenu(WidgetPtr widget) {
        if (!widget) return;

        // Create child widget submenu
        if (m_config.allowCreation) {
            if (ImGui::BeginMenu("Create Child")) {
                WidgetsRegistry& registry = getWidgetsRegistry();

                for (const auto& widgetTemplate : registry.getWidgetTemplates()) {
                    if (ImGui::MenuItem(widgetTemplate.name.c_str())) {
                        registry.createWidget(widget, widgetTemplate.name, widgetTemplate.type, false);
                        markFullRebuildNeeded();
                    }
                }

                ImGui::EndMenu();
            }
            ImGui::Separator();
        }

        // Rename option
        if (m_config.allowRenaming) {
            std::string name = widget->m_name;
            if (ImGuiUtils::beginMenuWithInputMenuField("Rename", "Name", name)) {
                widget->m_name = name;

                auto it = m_nodeCache.find(widget);
                if (it != m_nodeCache.end()) {
                    it->second.name = name;
                }
            }
            ImGui::Separator();
        }

        // Delete option
        if (m_config.allowDeletion) {
            if (ImGui::MenuItem("Delete Widget")) {
                // TODO: Implement widget deletion through WidgetsRegistry
            }
        }

        // Duplicate option
        if (ImGui::MenuItem("Duplicate")) {
            // TODO: Implement widget duplication
        }
    }

    void WidgetHierarchyPanel::reparentNode(WidgetPtr widget, WidgetPtr newParent, WidgetPtr insertBefore) {
        if (!widget) return;

        WidgetsRegistry& registry = getWidgetsRegistry();

        // Calculate new sibling index
        int newSiblingIndex = 0;
        if (newParent) {
            auto& siblings = newParent->m_children;
            if (insertBefore) {
                auto it = std::find(siblings.begin(), siblings.end(), insertBefore);
                newSiblingIndex = static_cast<int>(std::distance(siblings.begin(), it));
            } else {
                newSiblingIndex = static_cast<int>(siblings.size());
            }
        } else {
            const auto& roots = registry.getRootWidgets();
            if (insertBefore) {
                auto it = std::find(roots.begin(), roots.end(), insertBefore);
                newSiblingIndex = static_cast<int>(std::distance(roots.begin(), it));
            } else {
                newSiblingIndex = static_cast<int>(roots.size());
            }
        }

        registry.performWidgetMove(widget, newParent, newSiblingIndex);
    }

    void WidgetHierarchyPanel::processQueuedOperations() {
        // Widget operations are typically handled through WidgetsRegistry command queue
    }

    WidgetsRegistry& WidgetHierarchyPanel::getWidgetsRegistry() const {
        return m_appSystemsRegistry.getSystem<WidgetsRegistry>();
    }

}
