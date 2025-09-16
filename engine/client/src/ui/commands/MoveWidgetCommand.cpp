#include "MoveWidgetCommand.hpp"
#include "ui/WidgetsRegistry.hpp"

namespace TechEngine {
    MoveWidgetCommand::MoveWidgetCommand(WidgetsRegistry& registry,
                                         std::shared_ptr<Widget> subject,
                                         std::shared_ptr<Widget> newParent,
                                         int newSiblingIndex) : m_registry(registry),
                                                                m_subject(subject),
                                                                m_newParent(newParent),
                                                                m_newSiblingIndex(newSiblingIndex) {
    }

    void MoveWidgetCommand::execute() {
        // Store the old state before making changes, so we can Unexecute
        m_oldParent = m_subject->m_parent;
        if (m_oldParent) {
            // Find the old sibling index
            auto& siblings = m_oldParent->m_children;
            auto it = std::find(siblings.begin(), siblings.end(), m_subject);
            if (it != siblings.end()) {
                m_oldSiblingIndex = std::distance(siblings.begin(), it);
            }
        } else {
            // If it was a root widget, we need to know its old position among roots.
            // This is more complex, for now we can simplify. We'll handle it in the registry.
            m_oldSiblingIndex = -1;
        }

        // The actual logic is now delegated to the registry (the Model manager)
        m_registry.performWidgetMove(m_subject, m_newParent, m_newSiblingIndex);
    }

    void MoveWidgetCommand::unexecute() {
        // Use the stored old state to reverse the action
        m_registry.performWidgetMove(m_subject, m_oldParent, m_oldSiblingIndex);
    }
}
