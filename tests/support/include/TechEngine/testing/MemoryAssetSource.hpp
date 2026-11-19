#pragma once

#include <TechEngine/core/assets/AssetSource.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>

#include <cstddef>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace TechEngineTests {
    class MemoryAssetSource final : public TechEngine::AssetSource {
    private:
        std::unordered_map<TechEngine::ResourceId, std::vector<std::byte>> m_assets;
        std::unordered_set<TechEngine::ResourceId> m_absentForRole;

    public:
        void addAsset(const TechEngine::ResourceId id, std::vector<std::byte> bytes) {
            m_assets[id] = std::move(bytes);
        }

        void addAbsentForRole(const TechEngine::ResourceId id) {
            m_absentForRole.insert(id);
        }

        TechEngine::AssetAvailability find(const TechEngine::ResourceId id) const override {
            if (m_assets.contains(id)) {
                return TechEngine::AssetAvailability::Present;
            }
            if (m_absentForRole.contains(id)) {
                return TechEngine::AssetAvailability::AbsentForRole;
            }
            return TechEngine::AssetAvailability::Unknown;
        }

        std::error_code read(const TechEngine::ResourceId id, std::vector<std::byte>& out) const override {
            out = m_assets.at(id);
            return {};
        }
    };
}
