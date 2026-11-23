#pragma once

#include <TechEngine/app/Command.hpp>

#include <cstdint>
#include <string>
#include <string_view>

namespace TechEngine {
    class AssetIndex;
    class FileAccess;

    struct MaterialCreateRequest {
        std::string folder;
        std::string name;
    };

    class CreateMaterialCommand final : public Command {
    private:
        FileAccess* m_files = nullptr;
        AssetIndex* m_index = nullptr;
        MaterialCreateRequest m_request;

    public:
        CreateMaterialCommand(FileAccess& files, AssetIndex& index, MaterialCreateRequest request);

        // A suffix of 0 adds nothing, and 1 turns Brick into Brick_1.tematerial.
        static std::string outputPath(std::string_view folder, std::string_view name, std::uint32_t suffix);

        // Writes a default material at the first free outputPath, then lists the assets again. Two creates at once never
        // share a file.
        void apply(Scene& scene) override;
    };
}
