#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/AssetNaming.hpp>
#include <edit/CreateFolderCommand.hpp>
#include <edit/RefreshAssetsCommand.hpp>

#include <system_error>
#include <utility>

namespace TechEngine {
    CreateFolderCommand::CreateFolderCommand(FileAccess& files, AssetIndex& index, FolderCreateRequest request) : m_files(&files), m_index(&index), m_request(std::move(request)) {
    }

    std::string CreateFolderCommand::outputPath(const std::string_view folder, const std::string_view name, const std::uint32_t suffix) {
        return AssetNaming::join(folder, AssetNaming::withSuffix(name, suffix));
    }

    void CreateFolderCommand::apply(Scene& scene) {
        if (!AssetNaming::isValidName(m_request.name)) {
            TE_LOGGER_WARN("A folder cannot be named '{0}', so none is created in {1}", m_request.name, m_request.folder);
            return;
        }
        FileStatus parent;
        const std::error_code parentError = m_files->status(m_request.folder, parent);
        if (parentError || !parent.isDirectory) {
            TE_LOGGER_WARN("{0} is not a folder any more, so no folder is created in it", m_request.folder);
            return;
        }

        const auto candidate = [this](const std::uint32_t suffix) {
            return outputPath(m_request.folder, m_request.name, suffix);
        };
        const auto create = [this](const std::string& path) {
            return m_files->createDirectory(path);
        };
        std::string path;
        const std::error_code createError = AssetNaming::claimFirstFree(candidate, create, path);
        if (createError) {
            TE_LOGGER_ERROR("Failed to create the folder {0} in {1}: {2}", m_request.name, m_request.folder, createError.message());
            return;
        }
        TE_LOGGER_INFO("Created the folder {0}", path);
        RefreshAssetsCommand(*m_files, *m_index).apply(scene);
    }
}
