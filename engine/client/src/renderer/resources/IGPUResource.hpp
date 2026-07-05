#pragma once
#include "TechEngine/core/core/UUID.hpp"

namespace TechEngine {
    class IGPUResource {
    private:
        const UUID& m_resourceUUID;

    public:
        IGPUResource(const UUID& resourceUUID);


        const UUID& getUUID() const;
    };
}
