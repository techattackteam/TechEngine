#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>

#include <assets/AssetIndex.hpp>
#include <edit/RefreshAssetsCommand.hpp>

#include <system_error>

namespace TechEngine {
    RefreshAssetsCommand::RefreshAssetsCommand(const FileAccess& files, AssetIndex& index) : m_files(&files), m_index(&index) {
    }

    void RefreshAssetsCommand::apply(Scene&) {
        const std::error_code rebuildError = m_index->rebuild(*m_files);
        if (rebuildError) {
            TE_LOGGER_WARN("Failed to list the assets again, so the browser and the index keep the last listing: {0}", rebuildError.message());
        }
    }
}
