#pragma once

#include <TechEngine/app/Command.hpp>

namespace TechEngine {
    class AssetIndex;
    class FileAccess;

    class RefreshAssetsCommand final : public Command {
    private:
        const FileAccess* m_files = nullptr;
        AssetIndex* m_index = nullptr;

    public:
        RefreshAssetsCommand(const FileAccess& files, AssetIndex& index);

        void apply(Scene& scene) override;
    };
}
