#pragma once

#include <TechEngine/base/stringid/StringId.hpp>

namespace TechEngine {
    namespace internal {
        template<typename T>
        inline StringId g_resourceTypeTag{};
    }

    // Empty until a cache's registerLoader<T> runs.
    template<typename T>
    StringId resourceTypeTag() {
        return internal::g_resourceTypeTag<T>;
    }
}
