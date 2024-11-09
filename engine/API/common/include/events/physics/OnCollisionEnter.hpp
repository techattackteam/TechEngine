#pragma once

#include "common/include/core/ExportDLL.hpp"
#include "common/include/eventSystem/Event.hpp"

#include <cstdint>

namespace TechEngineAPI {
    class API_DLL OnCollisionEnter final : public Event {
    private:
        int32_t entity1;
        int32_t entity2;

    public:
        OnCollisionEnter(const int32_t entity1, const int32_t entity2) : entity1(entity1), entity2(entity2) {
        }

        [[nodiscard]] int32_t getEntity1() const {
            return entity1;
        }

        [[nodiscard]] int32_t getEntity2() const {
            return entity2;
        }
    };
}
