#pragma once

#pragma once
#include <cstdint>

#include "events/Event.hpp"

namespace TechEngine {
    class OnCollisionStay final : public Event {
    private:
        int32_t entity1;
        int32_t entity2;

    public:
        OnCollisionStay(const int32_t entity1, const int32_t entity2) : entity1(entity1), entity2(entity2) {
        }

        [[nodiscard]] int32_t getEntity1() const {
            return entity1;
        }

        [[nodiscard]] int32_t getEntity2() const {
            return entity2;
        }
    };
}
