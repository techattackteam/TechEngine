#pragma once

#include <TechEngine/core/scene/Entity.hpp>

#include <cstddef>
#include <string_view>

namespace TechEngine {
    class Hierarchy {
    private:
        friend class Scene;

        Entity m_parent;
        Entity m_firstChild;
        Entity m_nextSibling;
        Entity m_previousSibling;
        std::size_t m_childrenCount = 0;

    public:
        Hierarchy() = default;

        Hierarchy(const Hierarchy&) = default;

        Hierarchy(Hierarchy&&) noexcept = default;

        Hierarchy& operator=(const Hierarchy&) = default;

        Hierarchy& operator=(Hierarchy&&) noexcept = default;

        static constexpr std::string_view tag = "TechEngine::Hierarchy";
    };
}
