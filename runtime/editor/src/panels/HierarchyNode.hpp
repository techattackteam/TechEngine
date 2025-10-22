#pragma once
#include <memory>

#include "TechEngine/core/components/Entity.hpp"
#include "ui/Widget.hpp"

namespace TechEngine {
    struct HierarchyNode {
        HierarchyNode() : type(NodeType::None), depth(0), isOpen(false) {
        }

        ~HierarchyNode() {
            if (type == NodeType::Widget) {
                widget.~shared_ptr();
            }
        }


        // Copy constructor
        HierarchyNode(const HierarchyNode& other)
            : id(other.id), name(other.name), onSelect(other.onSelect),
              type(other.type), depth(other.depth), isOpen(other.isOpen) {
            if (type == NodeType::Widget) {
                widget = std::make_shared<Widget>(other.widget.get());
            } else if (type == NodeType::Entity) {
                entity = other.entity; // Just copy the ID
            } else if (type == NodeType::RenderPass) {
                renderPassName = new std::string(*other.renderPassName);
            }
        }

        // Move constructor
        HierarchyNode(HierarchyNode&& other) noexcept
            : id(other.id), name(std::move(other.name)), onSelect(std::move(other.onSelect)),
              type(other.type), depth(other.depth), isOpen(other.isOpen) {
            if (type == NodeType::Widget) {
                new(&widget) std::shared_ptr<Widget>(std::move(other.widget));
            } else if (type == NodeType::Entity) {
                entity = other.entity;
            } else if (type == NodeType::RenderPass) {
                renderPassName = std::move(other.renderPassName);
            }
        }

        // Copy assignment
        HierarchyNode& operator=(const HierarchyNode& other) {
            if (this != &other) {
                if (type == NodeType::Widget) {
                    widget.~shared_ptr();
                }
                id = other.id;
                name = other.name;
                onSelect = other.onSelect;
                type = other.type;
                depth = other.depth;
                isOpen = other.isOpen;
                if (type == NodeType::Widget) {
                    widget = std::make_shared<Widget>(other.widget.get());
                } else if (type == NodeType::Entity) {
                    entity = other.entity;
                } else if (type == NodeType::RenderPass) {
                    renderPassName = new std::string(*other.renderPassName);
                }
            }
            return *this;
        }

        // Move assignment
        HierarchyNode& operator=(HierarchyNode&& other) noexcept {
            if (this != &other) {
                if (type == NodeType::Widget) {
                    widget.~shared_ptr();
                }
                id = other.id;
                name = std::move(other.name);
                onSelect = std::move(other.onSelect);
                type = other.type;
                depth = other.depth;
                isOpen = other.isOpen;
                if (type == NodeType::Widget) {
                    new(&widget) std::shared_ptr<Widget>(std::move(other.widget));
                } else if (type == NodeType::Entity) {
                    entity = other.entity;
                } else if (type == NodeType::RenderPass) {
                    renderPassName = std::move(other.renderPassName);
                }
            }
            return *this;
        }

        // A unique ID for ImGui to track the node's state (e.g., open/closed)
        uint32_t id;
        std::string name;

        // A callback to execute when this node is selected
        std::function<void()> onSelect;

        // Enum to handle type-specific logic like Drag & Drop
        enum class NodeType { None, Entity, Widget, RenderPass };

        NodeType type = NodeType::None;

        int depth;
        bool isOpen;

        // Store the actual data payload
        union {
            Entity entity;
            std::shared_ptr<Widget> widget;
            std::string* renderPassName;
        };

        bool operator==(const HierarchyNode& other) const {
            if (type != other.type) return false;
            if (type == NodeType::Entity) return entity == other.entity;
            if (type == NodeType::Widget) return widget == other.widget;
            if (type == NodeType::RenderPass) return *renderPassName == *other.renderPassName;
            return true;
        }
    };
}
