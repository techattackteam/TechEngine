#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>

#include <algorithm>
#include <utility>

namespace TechEngine {
    void MeshRenderer::rebuildSlots(const Mesh& source, std::vector<std::string>& dropped) {
        std::vector<MaterialSlot> newSlots;
        newSlots.reserve(source.slots.size());
        for (const MeshSlot& slot: source.slots) {
            const auto kept = std::ranges::find(slots, slot.name, &MaterialSlot::name);
            if (kept != slots.end()) {
                newSlots.push_back(*kept);
            } else {
                newSlots.push_back(MaterialSlot{slot.name, AssetRef<Material>{slot.defaultMaterial, ResourceHandle<Material>{}}});
            }
        }

        for (const MaterialSlot& old: slots) {
            const auto stillThere = std::ranges::find(source.slots, old.name, &MeshSlot::name);
            if (stillThere == source.slots.end()) {
                dropped.push_back(old.name);
            }
        }
        slots = std::move(newSlots);
    }
}
