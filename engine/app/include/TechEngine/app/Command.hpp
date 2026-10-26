#pragma once

namespace TechEngine {
    class Scene;

    class Command {
    public:
        virtual ~Command() = default;

        // Built on main, applied on the simulation thread: a command owns copies of what it reads.
        virtual void apply(Scene& scene) = 0;
    };
}
