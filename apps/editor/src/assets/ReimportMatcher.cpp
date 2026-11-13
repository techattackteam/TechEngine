#include <assets/ReimportMatcher.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace TechEngine {
    // Exact keys are matched over every output before any name alone is, so an output whose index moved cannot take
    // the file of an unchanged output that shares its name.
    void ReimportMatcher::match(const std::span<const ImportKey> oldKeys, const std::span<const ImportKey> wanted, std::vector<std::uint32_t>& out) {
        out.assign(wanted.size(), REIMPORT_NO_MATCH);
        std::vector<bool> taken(oldKeys.size(), false);
        for (std::size_t i = 0; i < wanted.size(); i++) {
            for (std::size_t old = 0; old < oldKeys.size(); old++) {
                if (!taken[old] && sameTypeAndName(oldKeys[old], wanted[i]) && oldKeys[old].sourceIndex == wanted[i].sourceIndex) {
                    out[i] = static_cast<std::uint32_t>(old);
                    taken[old] = true;
                    break;
                }
            }
        }

        for (std::size_t i = 0; i < wanted.size(); i++) {
            if (out[i] != REIMPORT_NO_MATCH || wanted[i].sourceName.empty()) {
                continue;
            }
            std::uint32_t best = REIMPORT_NO_MATCH;
            for (std::size_t old = 0; old < oldKeys.size(); old++) {
                if (taken[old] || !sameTypeAndName(oldKeys[old], wanted[i])) {
                    continue;
                }
                if (best == REIMPORT_NO_MATCH || oldKeys[old].sourceIndex < oldKeys[best].sourceIndex) {
                    best = static_cast<std::uint32_t>(old);
                }
            }
            if (best != REIMPORT_NO_MATCH) {
                out[i] = best;
                taken[best] = true;
            }
        }
    }

    bool ReimportMatcher::sameTypeAndName(const ImportKey& first, const ImportKey& second) {
        return first.typeTag == second.typeTag && first.sourceName == second.sourceName;
    }
}
