#pragma once

#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/texture/Texture.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/ImageDecoder.hpp>
#include <assets/TextureImporter.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

struct aiMaterial;
struct aiScene;

namespace TechEngine {
    class FileAccess;

    inline constexpr std::uint32_t NO_PLANNED_OUTPUT = UINT32_MAX;

    struct SourceImage {
        // A file's path relative to the source file's folder, or an embedded image's own name or "*<index>". Empty when
        // the input has no image.
        std::string name;
        // The index in the scene's embedded images, or -1 for a file.
        int embeddedIndex = -1;

        bool operator==(const SourceImage&) const = default;
    };

    struct TextureOutput {
        // The file name it asks for, before clashes with the other outputs' names are settled.
        std::string name;
        std::string path;
        TexturePurpose purpose = TexturePurpose::BaseColor;
        // A packed ORM takes red from image and green and blue from secondImage, and is made from image alone when the
        // two are the same. Every other purpose reads image alone.
        SourceImage image;
        SourceImage secondImage;
        // The id, the import block and the sampler. The format, the size and the mips are filled when it is written.
        Texture texture;
    };

    struct MaterialOutput {
        std::string name;
        std::string path;
        // The id, the import block and the values. Each texture input is filled when the file is written.
        Material material;
        // Indices into MaterialPlan::textures, in the material's input order: base color, ORM, normal, emissive.
        std::array<std::uint32_t, 4> textures{NO_PLANNED_OUTPUT, NO_PLANNED_OUTPUT, NO_PLANNED_OUTPUT, NO_PLANNED_OUTPUT};
    };

    struct MaterialPlan {
        std::vector<MaterialOutput> materials;
        std::vector<TextureOutput> textures;
        // One entry per source material: its index in materials, or NO_PLANNED_OUTPUT.
        std::vector<std::uint32_t> materialBySource;

        // A null id for a source material that has no planned output.
        ResourceId materialId(unsigned int sourceIndex) const;
    };

    class MaterialImporter {
    private:
        FileAccess* m_files = nullptr;
        const aiScene* m_scene = nullptr;
        std::string m_sourcePath;
        std::string m_relativeSourcePath;
        bool m_gltfSource = false;

    public:
        // sourcePath is the source's virtual path, and relativeSourcePath what the import blocks name.
        MaterialImporter(FileAccess& files, const aiScene& scene, std::string_view sourcePath, std::string_view relativeSourcePath, bool gltfSource);

        // One material per source material whose used entry is true, and one texture per image, purpose and sampler they
        // sample. Names, keys and values are filled; paths and ids are left for the caller.
        void plan(const std::vector<bool>& used, MaterialPlan& out) const;

        // Writes every texture, then every material, into the paths the plan holds, and adds a record for each file. A
        // texture whose image cannot be read or decoded is logged and left out, and the inputs that sample it stay empty.
        std::error_code write(const MaterialPlan& plan, Side side, std::vector<AssetRecord>& records) const;

        // DDS through DdsReader, and PNG, JPEG and TGA through ImageDecoder.
        static std::error_code decodeImage(std::span<const std::byte> bytes, ByteImage& out);

    private:
        void planInputs(const aiMaterial& source, MaterialOutput& material, MaterialPlan& out) const;

        bool readImage(const aiMaterial& source, int textureType, SourceImage& image, TextureSampler& sampler) const;

        bool readMetallicRoughness(const aiMaterial& source, SourceImage& image, TextureSampler& sampler) const;

        bool loadTexture(const TextureOutput& output, ByteImage& out) const;

        std::error_code loadImage(const SourceImage& image, ByteImage& out) const;

        bool imagePath(std::string_view name, std::string& out) const;

        static void readValues(const aiMaterial& source, Material& out);

        static std::uint32_t addTexture(TexturePurpose purpose, const SourceImage& image, const SourceImage& secondImage, const TextureSampler& sampler, std::string_view relativeSourcePath, MaterialPlan& out);

        static std::string imageKeyName(const SourceImage& image, const SourceImage& secondImage, TexturePurpose purpose);

        static std::string imageStem(const SourceImage& image);

        static std::string decodeUri(std::string_view uri);

        static int hexDigit(char character);

        static TextureWrap toWrap(int mapMode);
    };
}
