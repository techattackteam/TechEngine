#pragma once

#include <TechEngine/core/scene/Entity.hpp>

#include <cstdint>

namespace TechEngine {
    class Scene;
    struct EditorSnapshot;

    class EditSession {
    private:
        Entity m_selected;
        std::uint64_t m_editRevision = 0;
        std::uint64_t m_savedRevision = 0;

    public:
        void setSelected(Entity entity);

        Entity getSelected() const;

        void recordSceneChange();

        void buildSnapshot(const Scene& scene, EditorSnapshot& out) const;
    };
}
