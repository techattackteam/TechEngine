#pragma once

#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>

#include <type_traits>

namespace TechEngine {
    template<typename T>
    struct AssetRef {
        ResourceId id;
        ResourceHandle<T> handle;

        bool operator==(const AssetRef&) const = default;
    };

    namespace internal {
        template<typename T>
        struct IsAssetRef : std::false_type {};

        template<typename T>
        struct IsAssetRef<AssetRef<T>> : std::true_type {};
    }

    template<typename T>
    concept AssetReference = internal::IsAssetRef<T>::value;
}
