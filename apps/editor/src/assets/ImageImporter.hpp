#pragma once

#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>

#include <assets/AssetIndex.hpp>

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace TechEngine {
    class FileAccess;
    struct FloatImage;

    struct ImageImportRequest {
        std::string sourcePath;
    };

    struct ImageImportResult {
        // The texture alone.
        std::vector<AssetRecord> records;
    };

    class ImageImporter {
    private:
        FileAccess* m_files = nullptr;
        std::function<ResourceId()> m_generateId;

    public:
        // generateId must return a new id on every call.
        ImageImporter(FileAccess& files, std::function<ResourceId()> generateId);

        static std::string outputPath(std::string_view sourcePath);

        // Writes nothing when it fails.
        std::error_code importFile(const ImageImportRequest& request, ImageImportResult& out) const;

    private:
        static bool isHdr(std::string_view sourcePath);

        static bool sideOf(std::string_view relativePath, Side& out);

        static char toLowerAscii(char character);

        static void packLevel(const FloatImage& image, std::vector<std::byte>& out);
    };
}
