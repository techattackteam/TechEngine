#pragma once

#include "core/CoreExportDLL.hpp"
#include "core/UUID.hpp"

namespace TechEngine {
    class CORE_DLL IResource {
    protected:
        UUID m_uuid;

    public:
        IResource() : m_uuid(UUID::generate()) {
        }

        virtual ~IResource() = default;

        const UUID& getUUID() const {
            return m_uuid;
        }
    };
}
