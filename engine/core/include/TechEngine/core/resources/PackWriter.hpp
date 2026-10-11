#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace TechEngine {
    struct PackInput {
        ResourceId id;
        StringId typeTag;
        std::string_view name;
        std::span<const std::byte> payload;
    };

    class PackWriter {
    private:
        std::vector<PackInput> m_entries;
        std::span<const std::byte> m_bootRecord;

    public:
        void add(const PackInput& input);

        void setBootRecord(std::span<const std::byte> bootRecord);

        void write(std::vector<std::byte>& out) const;
    };
}
