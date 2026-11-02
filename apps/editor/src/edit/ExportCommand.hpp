#pragma once

#include <TechEngine/app/Command.hpp>

#include <filesystem>

namespace TechEngine {
    class AssetIndex;
    class ComponentRegistry;
    class EditSession;
    class FileAccess;
    class Project;

    class ExportCommand final : public Command {
    private:
        const EditSession* m_session = nullptr;
        const Project* m_project = nullptr;
        const AssetIndex* m_index = nullptr;
        const FileAccess* m_files = nullptr;
        ComponentRegistry* m_registry = nullptr;
        std::filesystem::path m_runtimePath;
        std::filesystem::path m_exportFolder;

    public:
        ExportCommand(const EditSession& session, const Project& project, const AssetIndex& index, const FileAccess& files, ComponentRegistry& registry, std::filesystem::path runtimePath, std::filesystem::path exportFolder);

        // Exports the saved files, not the live scene.
        void apply(Scene& scene) override;
    };
}
