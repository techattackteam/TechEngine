#pragma once

#include <TechEngine/base/stringid/StringId.hpp>

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace TechEngine {
    inline constexpr std::uint32_t REIMPORT_NO_MATCH = UINT32_MAX;

    struct ImportKey {
        StringId typeTag;
        std::uint32_t sourceIndex = 0;
        std::string sourceName;
    };

    class ReimportMatcher {
    public:
        // Fills one entry per wanted key: the index of the old key whose file it keeps, or REIMPORT_NO_MATCH. An old key
        // is kept at most once, and only by a key of its own type. A name matches first, and among old keys of one name
        // the one with the same index wins, then the lowest index. An empty name matches an empty name at the same index.
        static void match(std::span<const ImportKey> oldKeys, std::span<const ImportKey> wanted, std::vector<std::uint32_t>& out);

    private:
        static bool sameTypeAndName(const ImportKey& first, const ImportKey& second);
    };
}
