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
        // The texture alone, or nothing for a re-import.
        std::vector<AssetRecord> records;
        // A re-import's texture, which keeps its id. Empty for an import.
        std::vector<ResourceId> rewritten;
    };

    class ImageImporter {
    private:
        FileAccess* m_files = nullptr;
        std::function<ResourceId()> m_generateId;

    public:
        // generateId must return a new id on every call.
        ImageImporter(FileAccess& files, std::function<ResourceId()> generateId);

        static std::string outputPath(std::string_view sourcePath);

        // Refuses an output that exists. Writes nothing when it fails.
        std::error_code importFile(const ImageImportRequest& request, ImageImportResult& out) const;

        // Rewrites the texture an earlier import wrote, under its id. Writes nothing when it fails.
        std::error_code reimportFile(const ImageImportRequest& request, ImageImportResult& out) const;

        static bool isHdr(std::string_view sourcePath);

        // relativePath is a virtual path's part after the alias, such as assets/client/Sky.hdr.
        static bool sideOf(std::string_view relativePath, Side& out);

    private:
        std::error_code importInto(const ImageImportRequest& request, bool reimport, ImageImportResult& out) const;

        static char toLowerAscii(char character);

        static void packLevel(const FloatImage& image, std::vector<std::byte>& out);
    };
}
