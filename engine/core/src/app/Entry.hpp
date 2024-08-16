#pragma once
#include <functional>


namespace TechEngine {
    class SystemsRegistry;
}

namespace TechEngine {
    class Entry {
    private:
        SystemsRegistry& m_systemsRegistry;

    public:
        explicit Entry(SystemsRegistry& systemsRegistry);

        void run(const std::function<void()>& onFixedUpdate, const std::function<void()>& onUpdate);
    };
}
