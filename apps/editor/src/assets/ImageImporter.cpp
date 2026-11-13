#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/texture/Texture.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/platform/files/VirtualPath.hpp>

#include <assets/ImageDecoder.hpp>
#include <assets/ImageImporter.hpp>
#include <assets/ImportedFile.hpp>
#include <assets/MipBuilder.hpp>
#include <assets/Rgb9E5.hpp>
#include <project/ProjectError.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    static constexpr std::string_view HDR_EXTENSION = ".hdr";
    static constexpr std::string_view TEXTURE_EXTENSION = ".tetexture";
    static constexpr std::string_view ASSETS_FOLDER = "assets/";
    static constexpr std::size_t CHANNELS = 4;
    static constexpr std::size_t PACKED_BYTES = 4;
    // A lat-long sky wraps around in longitude and ends at the poles.
    static constexpr TextureSampler SKY_SAMPLER{TextureWrap::Repeat, TextureWrap::Clamp, TextureFilter::Linear};

    ImageImporter::ImageImporter(FileAccess& files, std::function<ResourceId()> generateId) : m_files(&files), m_generateId(std::move(generateId)) {
    }

    std::string ImageImporter::outputPath(const std::string_view sourcePath) {
        const std::size_t slash = sourcePath.find_last_of('/');
        const std::size_t dot = sourcePath.find_last_of('.');
        const bool hasExtension = dot != std::string_view::npos && (slash == std::string_view::npos || dot > slash + 1);
        const std::string_view stem = hasExtension ? sourcePath.substr(0, dot) : sourcePath;
        return std::string(stem) + std::string(TEXTURE_EXTENSION);
    }

    std::error_code ImageImporter::importFile(const ImageImportRequest& request, ImageImportResult& out) const {
        return importInto(request, false, out);
    }

    std::error_code ImageImporter::reimportFile(const ImageImportRequest& request, ImageImportResult& out) const {
        return importInto(request, true, out);
    }

    std::error_code ImageImporter::importInto(const ImageImportRequest& request, const bool reimport, ImageImportResult& out) const {
        TE_PROFILER_FUNCTION();
        if (!isHdr(request.sourcePath)) {
            return ProjectError::UnsupportedImageType;
        }

        VirtualPathParts parts;
        const std::error_code splitError = splitVirtualPath(request.sourcePath, parts);
        if (splitError) {
            return splitError;
        }
        Side side = Side::Common;
        if (!sideOf(parts.relative, side)) {
            return ProjectError::SourceNotInAssets;
        }

        const std::string path = outputPath(request.sourcePath);
        FileStatus outputStatus;
        const std::error_code outputError = m_files->status(path, outputStatus);
        ResourceId keptId;
        if (!reimport) {
            if (!outputError) {
                return ProjectError::ImportOutputExists;
            }
            if (outputError != FileError::NotFound) {
                return outputError;
            }
        } else {
            if (outputError == FileError::NotFound) {
                return ProjectError::ReimportTargetMissing;
            }
            if (outputError) {
                return outputError;
            }
            ImportedFile existing;
            const std::error_code existingError = readImportedFile(*m_files, path, existing);
            if (existingError) {
                return existingError;
            }
            if (existing.typeTag != TEXTURE_TYPE_TAG) {
                return ProjectError::ReimportTargetMissing;
            }
            keptId = existing.id;
        }

        std::vector<std::byte> sourceBytes;
        const std::error_code readError = m_files->read(request.sourcePath, sourceBytes);
        if (readError) {
            return readError;
        }
        FloatImage image;
        const std::error_code decodeError = ImageDecoder::decodeFloat(sourceBytes, image);
        if (decodeError) {
            return decodeError;
        }

        Texture texture;
        texture.id = reimport ? keptId : m_generateId();
        texture.importBlock.sourcePath = std::string(parts.relative);
        texture.format = TextureFormat::Rgb9E5;
        texture.width = image.width;
        texture.height = image.height;
        texture.sampler = SKY_SAMPLER;

        std::vector<FloatImage> chain;
        MipBuilder::build(image, chain);
        texture.mips.resize(chain.size() + 1);
        packLevel(image, texture.mips[0]);
        for (std::size_t level = 0; level < chain.size(); level++) {
            packLevel(chain[level], texture.mips[level + 1]);
        }

        std::vector<std::byte> bytes;
        texture.write(bytes);
        const std::error_code writeError = m_files->write(path, bytes);
        if (writeError) {
            return writeError;
        }

        out.records.clear();
        out.rewritten.clear();
        if (reimport) {
            out.rewritten.push_back(texture.id);
        } else {
            out.records.push_back(AssetRecord{texture.id, TEXTURE_TYPE_TAG, side, path});
        }
        return {};
    }

    void ImageImporter::packLevel(const FloatImage& image, std::vector<std::byte>& out) {
        const std::size_t pixelCount = static_cast<std::size_t>(image.width) * image.height;
        out.resize(pixelCount * PACKED_BYTES);
        for (std::size_t pixel = 0; pixel < pixelCount; pixel++) {
            const float* source = &image.pixels[pixel * CHANNELS];
            const std::uint32_t packed = Rgb9E5::pack(source[0], source[1], source[2]);
            for (std::size_t byte = 0; byte < PACKED_BYTES; byte++) {
                out[pixel * PACKED_BYTES + byte] = static_cast<std::byte>((packed >> (8 * byte)) & 0xFF);
            }
        }
    }

    bool ImageImporter::isHdr(const std::string_view sourcePath) {
        const std::size_t slash = sourcePath.find_last_of('/');
        const std::size_t dot = sourcePath.find_last_of('.');
        if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash)) {
            return false;
        }
        std::string extension(sourcePath.substr(dot));
        std::ranges::transform(extension, extension.begin(), toLowerAscii);
        return extension == HDR_EXTENSION;
    }

    bool ImageImporter::sideOf(const std::string_view relativePath, Side& out) {
        if (!relativePath.starts_with(ASSETS_FOLDER)) {
            return false;
        }
        const std::string_view inside = relativePath.substr(ASSETS_FOLDER.size());
        const std::size_t slash = inside.find('/');
        if (slash == std::string_view::npos) {
            return false;
        }
        static constexpr std::array<std::pair<std::string_view, Side>, 3> SIDES{{{"common", Side::Common}, {"client", Side::Client}, {"server", Side::Server}}};
        const std::string_view name = inside.substr(0, slash);
        for (const std::pair<std::string_view, Side>& side: SIDES) {
            if (side.first == name) {
                out = side.second;
                return true;
            }
        }
        return false;
    }

    char ImageImporter::toLowerAscii(const char character) {
        if (character >= 'A' && character <= 'Z') {
            return static_cast<char>(character - 'A' + 'a');
        }
        return character;
    }
}
