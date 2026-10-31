#pragma once

#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <edit/EditRecord.hpp>

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace TechEngine {
    class ComponentRegistry;
    class Scene;
    struct EditorSnapshot;

    struct SceneFile {
        ResourceId id;
        std::string path;
    };

    class EditSession {
    private:
        Entity m_selected;
        // Set only while the live scene is the one in this file, so a save never overwrites a file the editor did not load.
        SceneFile m_sceneFile;
        std::vector<EditRecord> m_records;
        std::uint64_t m_editRevision = 0;
        std::uint64_t m_savedRevision = 0;

    public:
        void setSelected(Entity entity);

        Entity getSelected() const;

        void recordSceneChange();

        void recordSave();

        void setSceneFile(SceneFile file);

        bool getSceneFile(SceneFile& out) const;

        void record(EditRecord record);

        std::span<const EditRecord> getRecords() const;

        void buildSnapshot(const Scene& scene, const ComponentRegistry& registry, EditorSnapshot& out) const;

    private:
        void collectComponents(const Scene& scene, const ComponentRegistry& registry, EditorSnapshot& out) const;
    };
}
