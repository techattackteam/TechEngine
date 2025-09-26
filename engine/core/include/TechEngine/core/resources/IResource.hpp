#pragma once

#include "TechEngine/core/core/CoreExportDLL.hpp"
#include "TechEngine/core/core/UUID.hpp"

//TODO: Make this private
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
