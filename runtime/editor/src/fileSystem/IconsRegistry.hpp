#pragma once
#include <filesystem>
#include <unordered_map>
#include "imgui.h"
#include "fileSystem/FileSystem.hpp"
#include "TechEngine/core/core/UUID.hpp"
#include "TechEngine/core/resources/texture/TextureResource.hpp"

namespace TechEngine {
    struct IconEntry {
        ImTextureID id = nullptr;
        glm::vec2 size = {0.0f, 0.0f};
    };

    class IconRegistry {
    public:
        explicit IconRegistry(FileSystem& fileSystem);

        ~IconRegistry();

        IconEntry getIcon(const std::filesystem::path& virtualIconPath);

        bool buildThumbnailFromPixelData(UUID resourceUUID, const TextureResource& texture);

        void setThumbnail(UUID resourceUUID, IconEntry texture);

        IconEntry getThumbnail(UUID resourceUUID, const std::filesystem::path& fallbackIconPath);

        bool hasThumbnail(UUID resourceUUID) const;

        void releaseThumbnail(UUID resourceUUID);

        void releaseAll();

    private:
        FileSystem& m_fileSystem;

        std::unordered_map<std::filesystem::path, IconEntry> m_iconCache;

        std::unordered_map<UUID, IconEntry> m_thumbnailCache;

        IconEntry loadFromDisk(const std::filesystem::path& virtualIconPath) const;

        void deleteGLTexture(ImTextureID id);
    };
}
