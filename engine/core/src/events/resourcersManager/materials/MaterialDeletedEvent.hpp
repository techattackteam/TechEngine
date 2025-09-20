#pragma once

#include "core/CoreExportDLL.hpp"
#include <string>

namespace TechEngine {
    class CORE_DLL MaterialDeletedEvent : public Event{
    private:
        std::string m_name;

    public:
        explicit MaterialDeletedEvent(std::string name) : m_name(std::move(name)) {
        }

        const std::string& getName() const {
            return m_name;
        }
    };
}
