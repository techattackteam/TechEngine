#pragma once
#include "ICommand.hpp"
#include <memory>

#include "ui/WidgetsRegistry.hpp"

namespace TechEngine {
    class CLIENT_DLL MoveWidgetCommand : public ICommand {
    private:
        WidgetsRegistry& m_registry;
        std::shared_ptr<Widget> m_subject;
        std::shared_ptr<Widget> m_newParent;
        int m_newSiblingIndex;

        // State for undo
        std::shared_ptr<Widget> m_oldParent;
        int m_oldSiblingIndex = -1;

    public:
        MoveWidgetCommand(WidgetsRegistry& registry, std::shared_ptr<Widget> subject, std::shared_ptr<Widget> newParent, int newSiblingIndex);

        void execute() override;

        void unexecute() override;
    };
}
