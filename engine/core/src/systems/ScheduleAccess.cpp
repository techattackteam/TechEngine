#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/systems/ScheduleAccess.hpp>

namespace TechEngine {
    ScheduleAccess::ScheduleAccess(const ComponentRegistry&, std::span<const ComponentTypeId>, std::span<const ComponentTypeId>) {
        // TODO(S6-T6): lower declared types to dense read/write masks.
    }

    bool ScheduleAccess::reads(const ComponentDenseId) const {
        return false;
    }

    bool ScheduleAccess::writes(const ComponentDenseId) const {
        return false;
    }

    bool ScheduleAccess::touches(const ComponentDenseId) const {
        return false;
    }
}
