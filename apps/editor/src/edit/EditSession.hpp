#pragma once

#include <TechEngine/core/scene/Entity.hpp>

#include <edit/EditRecord.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace TechEngine {
    class ComponentRegistry;
    class Scene;
    struct EditorSnapshot;

    class EditSession {
    private:
        Entity m_selected;
        std::vector<EditRecord> m_records;
        std::uint64_t m_editRevision = 0;
        std::uint64_t m_savedRevision = 0;

    public:
        void setSelected(Entity entity);

        Entity getSelected() const;

        void recordSceneChange();

        void record(EditRecord record);

        std::span<const EditRecord> getRecords() const;

        void buildSnapshot(const Scene& scene, const ComponentRegistry& registry, EditorSnapshot& out) const;

    private:
        void collectComponents(const Scene& scene, const ComponentRegistry& registry, EditorSnapshot& out) const;
    };
}
