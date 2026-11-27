#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>

#include <assets/index/AssetIndex.hpp>
#include <edit/assets/DeleteAssetsCommand.hpp>
#include <edit/assets/DeletePreview.hpp>
#include <edit/assets/PreviewDeleteCommand.hpp>
#include <edit/session/EditSession.hpp>

#include <algorithm>
#include <cstddef>
#include <utility>

namespace TechEngine {
    PreviewDeleteCommand::PreviewDeleteCommand(
        EditSession& session, ComponentRegistry& registry, const FileAccess& files, const ResourceCache& resources, const AssetIndex& index, JobSystem& jobs, const std::vector<BatchId>& createBatches, std::vector<std::string> paths, const std::uint64_t sequence)
        : m_session(&session), m_registry(&registry), m_files(&files), m_resources(&resources), m_index(&index), m_jobs(&jobs), m_createBatches(&createBatches), m_paths(std::move(paths)), m_sequence(sequence) {
    }

    void PreviewDeleteCommand::apply(Scene& scene) {
        DeletePreview preview;
        preview.sequence = m_sequence;
        preview.paths = m_paths;
        preview.refusal = DeleteAssetsCommand::findRefusal(*m_session, *m_jobs, *m_createBatches, m_paths);

        for (const ListedEntry& entry: m_index->getListing()) {
            if (entry.kind == ListedKind::Folder) {
                continue;
            }
            const bool held = std::ranges::any_of(m_paths, [&entry](const std::string& path) {
                return entry.path != path && DeleteAssetsCommand::isAtOrUnder(entry.path, path);
            });
            if (held) {
                preview.heldFileCount++;
            }
        }

        std::vector<AssetRecord> deleted;
        DeleteAssetsCommand::collectDeletedRecords(*m_index, m_paths, deleted);
        std::vector<ResourceId> deletedIds;
        for (const AssetRecord& record: deleted) {
            deletedIds.push_back(record.id);
        }
        std::ranges::sort(deletedIds);

        std::vector<std::string> scenes;
        std::vector<std::string> others;
        std::vector<ResourceId> references;
        if (!deletedIds.empty()) {
            SceneFile sceneFile;
            const bool hasSceneFile = m_session->getSceneFile(sceneFile);
            for (const AssetRecord& record: m_index->getRecords()) {
                if (DeleteAssetsCommand::isAtOrUnderAny(record.path, m_paths)) {
                    continue;
                }
                const std::error_code readError = readReferences(record, references);
                if (readError) {
                    TE_LOGGER_WARN("The delete warning cannot read {0}, so it may miss a file that uses the selection: {1}", record.path, readError.message());
                    continue;
                }
                // The open scene's unsaved edits are not in its file, and the delete empties them too.
                if (hasSceneFile && record.path == sceneFile.path) {
                    std::vector<ResourceId> liveReferences;
                    const SceneAsset liveAsset{};
                    liveAsset.collectReferences(scene, liveReferences);
                    references.insert(references.end(), liveReferences.begin(), liveReferences.end());
                }
                const bool names = std::ranges::any_of(references, [&deletedIds](const ResourceId reference) {
                    return std::ranges::binary_search(deletedIds, reference);
                });
                if (!names) {
                    continue;
                }
                if (record.typeTag == SCENE_TYPE_TAG) {
                    scenes.push_back(record.path);
                } else {
                    others.push_back(record.path);
                }
            }
        }
        std::ranges::sort(scenes);
        std::ranges::sort(others);
        preview.users = std::move(scenes);
        preview.users.insert(preview.users.end(), others.begin(), others.end());

        m_session->setDeletePreview(std::move(preview));
    }

    std::error_code PreviewDeleteCommand::readReferences(const AssetRecord& record, std::vector<ResourceId>& out) const {
        out.clear();
        const bool holdsReferences = record.typeTag == SCENE_TYPE_TAG || record.typeTag == MODEL_TYPE_TAG || record.typeTag == MESH_TYPE_TAG || record.typeTag == MATERIAL_TYPE_TAG;
        if (!holdsReferences) {
            return {};
        }

        std::vector<std::byte> bytes;
        const std::error_code readError = m_files->read(record.path, bytes);
        if (readError) {
            return readError;
        }

        if (record.typeTag == SCENE_TYPE_TAG) {
            Scene parsed(*m_registry);
            SceneAsset asset;
            const std::error_code parseError = asset.read(bytes, parsed);
            if (parseError) {
                return parseError;
            }
            asset.collectReferences(parsed, out);
            return {};
        }

        // The editor's cache registers no model loader, so a model cannot go through listDependencies.
        if (record.typeTag == MODEL_TYPE_TAG) {
            Model model;
            const std::error_code modelError = model.read(bytes);
            if (modelError) {
                return modelError;
            }
            for (const ModelNode& node: model.nodes) {
                if (node.mesh.valid()) {
                    out.push_back(node.mesh);
                }
            }
            return {};
        }

        return m_resources->listDependencies(bytes, out);
    }
}
