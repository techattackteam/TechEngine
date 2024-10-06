#pragma once
#include "core/CoreExportDLL.hpp"
#include <functional>

namespace TechEngine {
    class SystemsRegistry;

    class CORE_DLL Entry {
    private:
        SystemsRegistry& m_systemsRegistry;

    public:
        explicit Entry(SystemsRegistry& systemsRegistry);

        Entry(const Entry& rhs);

        void run(const std::function<void()>& onFixedUpdate, const std::function<void()>& onUpdate);
    };
}

