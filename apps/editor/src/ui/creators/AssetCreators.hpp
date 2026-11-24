#pragma once

#include <TechEngine/core/assets/ImportBlock.hpp>

#include <assets/import/ModelImporter.hpp>
#include <ui/creators/AssetCreator.hpp>

#include <cstdint>
#include <string>
#include <string_view>

namespace TechEngine {
    class ModelCreator final : public AssetCreator {
    private:
        ImportMode m_mode = ImportMode::Model;
        ImportSettings m_settings;

    public:
        explicit ModelCreator(ImportMode mode);

        const char* getLabel() const override;

        bool accepts(const CreateContext& context) const override;

        std::string getDefaultName(const CreateContext& context) const override;

        std::string getOutputPath(const CreateContext& context, std::string_view name, std::uint32_t suffix) const override;

        void begin(const CreateContext& context) override;

        void drawSettings() override;

        void submit(EditorCommands& commands, const CreateContext& context, std::string name) override;
    };

    class SkyCreator final : public AssetCreator {
    public:
        const char* getLabel() const override;

        bool accepts(const CreateContext& context) const override;

        std::string getDefaultName(const CreateContext& context) const override;

        std::string getOutputPath(const CreateContext& context, std::string_view name, std::uint32_t suffix) const override;

        void submit(EditorCommands& commands, const CreateContext& context, std::string name) override;
    };

    class FolderCreator final : public AssetCreator {
    public:
        const char* getLabel() const override;

        bool accepts(const CreateContext& context) const override;

        std::string getDefaultName(const CreateContext& context) const override;

        std::string getOutputPath(const CreateContext& context, std::string_view name, std::uint32_t suffix) const override;

        void submit(EditorCommands& commands, const CreateContext& context, std::string name) override;
    };

    // Not on the server side, because the inspector's picker refuses a server asset, so it could never be assigned.
    class MaterialCreator final : public AssetCreator {
    public:
        const char* getLabel() const override;

        bool accepts(const CreateContext& context) const override;

        std::string getDefaultName(const CreateContext& context) const override;

        std::string getOutputPath(const CreateContext& context, std::string_view name, std::uint32_t suffix) const override;

        void submit(EditorCommands& commands, const CreateContext& context, std::string name) override;
    };
}
