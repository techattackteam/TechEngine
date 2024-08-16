#pragma once

namespace TechEngine {
    class System {
    protected:
        System() = default;

        friend class SystemsRegistry;

    public:
        System(const System&) = delete;

        System& operator=(const System&) = delete;

        virtual ~System() = default;

        virtual void init() = 0;

        virtual void onStart() = 0;

        virtual void onFixedUpdate();

        virtual void onUpdate();

        virtual void onStop() = 0;

        virtual void destroy() = 0;
    };
}
