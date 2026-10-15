#pragma once

#include <TechEngine/core/scene/ComponentRegistry.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace TechEngineTests {
    template<std::size_t Index>
    struct IndexedComponent {
        std::uint32_t value = 0;
    };

    template<std::size_t... Indices>
    static void registerIndexedComponents(TechEngine::ComponentRegistry& registry, std::index_sequence<Indices...>) {
        (registry.registerComponent<IndexedComponent<Indices>>("Test.IndexedComponent." + std::to_string(Indices)), ...);
    }
}
