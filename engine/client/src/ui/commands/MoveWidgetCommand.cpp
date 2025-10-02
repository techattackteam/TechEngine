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
        m_oldParent = m_subject->m_parent;
        if (m_oldParent) {
            auto& siblings = m_oldParent->m_children;
            auto it = std::find(siblings.begin(), siblings.end(), m_subject);
            if (it != siblings.end()) {
                m_oldSiblingIndex = std::distance(siblings.begin(), it);
            }
        } else {
            m_oldSiblingIndex = -1;
        }

        m_registry.performWidgetMove(m_subject, m_newParent, m_newSiblingIndex);
    }

    void MoveWidgetCommand::unexecute() {
        m_registry.performWidgetMove(m_subject, m_oldParent, m_oldSiblingIndex);
    }
}
