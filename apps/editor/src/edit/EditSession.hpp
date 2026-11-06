#pragma once

#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <edit/EditRecord.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace TechEngine {
    class ComponentRegistry;
    class Scene;
    struct ComponentRow;
    struct ComponentTypeRecord;
    struct EditorSnapshot;

    struct SceneFile {
        ResourceId id;
        std::string path;
    };

    class EditSession {
    private:
        std::vector<Entity> m_selection;
        Entity m_active;
        std::uint64_t m_selectionSequence = 0;
        // Set only while the live scene is the one in this file, so a save never overwrites a file the editor did not load.
        SceneFile m_sceneFile;
        std::vector<EditRecord> m_records;
        std::uint64_t m_editRevision = 0;
        std::uint64_t m_savedRevision = 0;

    public:
        // A sequence older than the last one applied changes nothing.
        void setSelection(std::vector<Entity> selection, Entity active, std::uint64_t sequence);

        std::span<const Entity> getSelection() const;

        Entity getActive() const;

        std::uint64_t getSelectionSequence() const;

        void removeDeadFromSelection(const Scene& scene);

        void recordSceneChange();

        void recordSave();

        bool hasUnsavedEdits() const;

        void setSceneFile(SceneFile file);

        bool getSceneFile(SceneFile& out) const;

        void record(EditRecord record);

        std::span<const EditRecord> getRecords() const;

        void buildSnapshot(const Scene& scene, const ComponentRegistry& registry, EditorSnapshot& out) const;

    private:
        void collectComponents(const Scene& scene, const ComponentRegistry& registry, EditorSnapshot& out) const;

        void markMixedFields(const Scene& scene, const ComponentTypeRecord& record, ComponentRow& row) const;

        void collectWorldTransform(const Scene& scene, EditorSnapshot& out) const;

        void markMixedRotationAxes(const Scene& scene, bool world, std::array<bool, 3>& out) const;
    };
}
