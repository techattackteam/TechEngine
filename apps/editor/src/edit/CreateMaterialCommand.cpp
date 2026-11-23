#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/AssetNaming.hpp>
#include <edit/CreateMaterialCommand.hpp>
#include <edit/RefreshAssetsCommand.hpp>

#include <cstddef>
#include <span>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    static constexpr std::string_view MATERIAL_EXTENSION = ".tematerial";

    CreateMaterialCommand::CreateMaterialCommand(FileAccess& files, AssetIndex& index, MaterialCreateRequest request) : m_files(&files), m_index(&index), m_request(std::move(request)) {
    }

    std::string CreateMaterialCommand::outputPath(const std::string_view folder, const std::string_view name, const std::uint32_t suffix) {
        return AssetNaming::join(folder, AssetNaming::withSuffix(name, suffix)) + std::string(MATERIAL_EXTENSION);
    }

    void CreateMaterialCommand::apply(Scene& scene) {
        if (!AssetNaming::isValidName(m_request.name)) {
            TE_LOGGER_WARN("A material cannot be named '{0}', so none is created in {1}", m_request.name, m_request.folder);
            return;
        }

        Material material;
        material.id = m_index->generateId();
        std::vector<std::byte> bytes;
        material.write(bytes);

        const auto candidate = [this](const std::uint32_t suffix) {
            return outputPath(m_request.folder, m_request.name, suffix);
        };
        const auto create = [this, &bytes](const std::string& path) {
            return m_files->createFile(path, std::span<const std::byte>(bytes));
        };
        std::string path;
        const std::error_code createError = AssetNaming::claimFirstFree(candidate, create, path);
        if (createError) {
            TE_LOGGER_ERROR("Failed to create the material {0} in {1}: {2}", m_request.name, m_request.folder, createError.message());
            return;
        }
        TE_LOGGER_INFO("Created the material {0}", path);
        RefreshAssetsCommand(*m_files, *m_index).apply(scene);
    }
}
