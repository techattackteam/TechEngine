#include <assets/import/TextureImporter.hpp>
#include <assets/index/AssetNaming.hpp>
#include <edit/EditorCommands.hpp>
#include <edit/assets/CreateFolderCommand.hpp>
#include <edit/assets/CreateMaterialCommand.hpp>
#include <ui/creators/AssetCreators.hpp>

#include <imgui.h>

#include <utility>

namespace TechEngine {
    static constexpr float SCALE_INPUT_WIDTH_EMS = 8.0f;
    static constexpr std::string_view DEFAULT_FOLDER_NAME = "New Folder";
    static constexpr std::string_view DEFAULT_MATERIAL_NAME = "Material";

    ModelCreator::ModelCreator(const ImportMode mode) : m_mode(mode) {
    }

    const char* ModelCreator::getLabel() const {
        return m_mode == ImportMode::MergedMesh ? "Create mesh..." : "Create model...";
    }

    bool ModelCreator::accepts(const CreateContext& context) const {
        return !context.sourcePath.empty() && context.sourceKind == ListedKind::ModelSource;
    }

    std::string ModelCreator::getDefaultName(const CreateContext& context) const {
        return std::string(AssetNaming::stemOf(context.sourcePath));
    }

    std::string ModelCreator::getOutputPath(const CreateContext& context, const std::string_view name, const std::uint32_t suffix) const {
        return ModelImporter::importFolder(context.sourcePath, name, suffix);
    }

    void ModelCreator::begin(const CreateContext&) {
        m_settings = {};
    }

    void ModelCreator::drawSettings() {
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * SCALE_INPUT_WIDTH_EMS);
        ImGui::InputFloat("Scale", &m_settings.scale, 0.0f, 0.0f, "%g");
        ImGui::TextUnformatted("Up axis");
        ImGui::SameLine();
        if (ImGui::RadioButton("Y", m_settings.upAxis == UpAxis::Y)) {
            m_settings.upAxis = UpAxis::Y;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Z", m_settings.upAxis == UpAxis::Z)) {
            m_settings.upAxis = UpAxis::Z;
        }
    }

    void ModelCreator::submit(EditorCommands& commands, const CreateContext& context, std::string name) {
        ModelImportRequest request;
        request.sourcePath = context.sourcePath;
        request.side = context.side;
        request.settings = m_settings;
        request.mode = m_mode;
        request.name = std::move(name);
        commands.createModel(std::move(request));
    }

    const char* SkyCreator::getLabel() const {
        return "Create sky...";
    }

    bool SkyCreator::accepts(const CreateContext& context) const {
        return !context.sourcePath.empty() && context.sourceKind == ListedKind::SkySource;
    }

    std::string SkyCreator::getDefaultName(const CreateContext& context) const {
        return std::string(AssetNaming::stemOf(context.sourcePath));
    }

    std::string SkyCreator::getOutputPath(const CreateContext& context, const std::string_view name, const std::uint32_t suffix) const {
        return TextureImporter::skyOutputPath(context.sourcePath, name, suffix);
    }

    void SkyCreator::submit(EditorCommands& commands, const CreateContext& context, std::string name) {
        SkyImportRequest request;
        request.sourcePath = context.sourcePath;
        request.name = std::move(name);
        commands.createSky(std::move(request));
    }

    const char* FolderCreator::getLabel() const {
        return "Create folder...";
    }

    bool FolderCreator::accepts(const CreateContext& context) const {
        return context.sourcePath.empty();
    }

    std::string FolderCreator::getDefaultName(const CreateContext&) const {
        return std::string(DEFAULT_FOLDER_NAME);
    }

    std::string FolderCreator::getOutputPath(const CreateContext& context, const std::string_view name, const std::uint32_t suffix) const {
        return CreateFolderCommand::outputPath(context.folder, name, suffix);
    }

    void FolderCreator::submit(EditorCommands& commands, const CreateContext& context, std::string name) {
        FolderCreateRequest request;
        request.folder = context.folder;
        request.name = std::move(name);
        commands.createFolder(std::move(request));
    }

    const char* MaterialCreator::getLabel() const {
        return "Create material...";
    }

    bool MaterialCreator::accepts(const CreateContext& context) const {
        return context.sourcePath.empty() && context.side != Side::Server;
    }

    std::string MaterialCreator::getDefaultName(const CreateContext&) const {
        return std::string(DEFAULT_MATERIAL_NAME);
    }

    std::string MaterialCreator::getOutputPath(const CreateContext& context, const std::string_view name, const std::uint32_t suffix) const {
        return CreateMaterialCommand::outputPath(context.folder, name, suffix);
    }

    void MaterialCreator::submit(EditorCommands& commands, const CreateContext& context, std::string name) {
        MaterialCreateRequest request;
        request.folder = context.folder;
        request.name = std::move(name);
        commands.createMaterial(std::move(request));
    }
}
