#pragma once

#include <TechEngine/app/Command.hpp>

#include <cstdint>
#include <string>
#include <string_view>

namespace TechEngine {
    class AssetIndex;
    class FileAccess;

    struct FolderCreateRequest {
        std::string folder;
        std::string name;
    };

    class CreateFolderCommand final : public Command {
    private:
        FileAccess* m_files = nullptr;
        AssetIndex* m_index = nullptr;
        FolderCreateRequest m_request;

    public:
        CreateFolderCommand(FileAccess& files, AssetIndex& index, FolderCreateRequest request);

        // A suffix of 0 adds nothing, and 1 turns Walls into Walls_1.
        static std::string outputPath(std::string_view folder, std::string_view name, std::uint32_t suffix);

        // Creates an empty folder at the first free outputPath, then lists the assets again. A parent folder that is gone
        // is refused, because creating the folder would bring it back.
        void apply(Scene& scene) override;
    };
}
