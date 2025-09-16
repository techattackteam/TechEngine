#pragma once

namespace TechEngine {
    class ICommand {
    public:
        virtual ~ICommand() = default;

        virtual void execute() = 0;

        virtual void unexecute() = 0; // For undo/redo functionality!
    };
}
