#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>

#include <assets/index/AssetIndex.hpp>
#include <edit/assets/AssetReferenceClearer.hpp>
#include <edit/assets/DeleteAssetsCommand.hpp>
#include <edit/assets/RefreshAssetsCommand.hpp>
#include <edit/session/EditSession.hpp>
#include <project/ProjectError.hpp>

#include <algorithm>
#include <cstddef>
#include <utility>

namespace TechEngine {
    DeleteAssetsCommand::DeleteAssetsCommand(EditSession& session, FileAccess& files, AssetIndex& index, JobSystem& jobs, const std::vector<BatchId>& createBatches, std::vector<std::string> paths)
        : m_session(&session), m_files(&files), m_index(&index), m_jobs(&jobs), m_createBatches(&createBatches), m_paths(std::move(paths)) {
    }

    std::error_code DeleteAssetsCommand::findRefusal(const EditSession& session, JobSystem& jobs, const std::span<const BatchId> createBatches, const std::span<const std::string> paths) {
        SceneFile sceneFile;
        const bool hasSceneFile = session.getSceneFile(sceneFile);
        if (hasSceneFile && isAtOrUnderAny(sceneFile.path, paths)) {
            return ProjectError::DeletesOpenScene;
        }

        const bool creating = std::ranges::any_of(createBatches, [&jobs](const BatchId batch) {
            return !jobs.isDone(batch);
        });
        if (creating) {
            return ProjectError::CreateInProgress;
        }
        return {};
    }

    void DeleteAssetsCommand::collectDeletedRecords(const AssetIndex& index, const std::span<const std::string> paths, std::vector<AssetRecord>& out) {
        out.clear();
        for (const AssetRecord& record: index.getRecords()) {
            if (isAtOrUnderAny(record.path, paths)) {
                out.push_back(record);
            }
        }
    }

    bool DeleteAssetsCommand::isAtOrUnder(const std::string_view path, const std::string_view root) {
        if (!path.starts_with(root)) {
            return false;
        }
        return path.size() == root.size() || path[root.size()] == '/';
    }

    bool DeleteAssetsCommand::isAtOrUnderAny(const std::string_view path, const std::span<const std::string> roots) {
        return std::ranges::any_of(roots, [path](const std::string& root) {
            return isAtOrUnder(path, root);
        });
    }

    void DeleteAssetsCommand::apply(Scene& scene) {
        const std::error_code refusal = findRefusal(*m_session, *m_jobs, *m_createBatches, m_paths);
        if (refusal) {
            TE_LOGGER_WARN("Nothing is deleted: {0}", refusal.message());
            return;
        }

        std::vector<ResourceId> removedIds;
        std::vector<AssetRecord> removedRecords;
        std::size_t removedCount = 0;
        for (const std::string& path: m_paths) {
            const std::error_code removeError = m_files->remove(path, true);
            if (removeError && removeError != FileError::NotFound) {
                TE_LOGGER_ERROR("Failed to delete {0}, so it and what it holds stay: {1}", path, removeError.message());
                continue;
            }
            if (!removeError) {
                removedCount++;
            }

            collectDeletedRecords(*m_index, std::span<const std::string>(&path, 1), removedRecords);
            for (const AssetRecord& record: removedRecords) {
                removedIds.push_back(record.id);
            }
        }
        TE_LOGGER_INFO("Deleted {0} of {1} selected paths, which held {2} indexed assets", removedCount, m_paths.size(), removedIds.size());

        AssetReferenceClearer clearer(removedIds);
        const SceneAsset sceneAsset{};
        sceneAsset.visitReferences(scene, clearer);
        if (clearer.getClearedCount() > 0) {
            TE_LOGGER_INFO("Emptied {0} references to deleted assets in the open scene", clearer.getClearedCount());
            m_session->recordSceneChange();
        }

        RefreshAssetsCommand(*m_files, *m_index).apply(scene);
    }
}
