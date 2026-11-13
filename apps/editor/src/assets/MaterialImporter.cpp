#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>

#include <assets/DdsReader.hpp>
#include <assets/MaterialImporter.hpp>
#include <project/ProjectError.hpp>

#include <assimp/GltfMaterial.h>
#include <assimp/material.h>
#include <assimp/scene.h>
#include <assimp/texture.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    static constexpr std::string_view UNNAMED_MATERIAL = "Material ";
    static constexpr std::string_view UNNAMED_TEXTURE = "Texture ";
    static constexpr std::string_view GLTF_ALPHA_MASK = "MASK";
    static constexpr std::string_view GLTF_ALPHA_BLEND = "BLEND";
    static constexpr int GLTF_NEAREST_FILTER = 9728;
    static constexpr std::size_t BASE_COLOR_INPUT = 0;
    static constexpr std::size_t OCCLUSION_ROUGHNESS_METALLIC_INPUT = 1;
    static constexpr std::size_t NORMAL_INPUT = 2;
    static constexpr std::size_t EMISSIVE_INPUT = 3;

    ResourceId MaterialPlan::materialId(const unsigned int sourceIndex) const {
        if (sourceIndex >= materialBySource.size() || materialBySource[sourceIndex] == NO_PLANNED_OUTPUT) {
            return ResourceId{};
        }
        return materials[materialBySource[sourceIndex]].material.id;
    }

    MaterialImporter::MaterialImporter(FileAccess& files, const aiScene& scene, const std::string_view sourcePath, const std::string_view relativeSourcePath, const bool gltfSource)
        : m_files(&files), m_scene(&scene), m_sourcePath(sourcePath), m_relativeSourcePath(relativeSourcePath), m_gltfSource(gltfSource) {
    }

    void MaterialImporter::plan(const std::vector<bool>& used, MaterialPlan& out) const {
        out.materialBySource.assign(m_scene->mNumMaterials, NO_PLANNED_OUTPUT);
        for (unsigned int i = 0; i < m_scene->mNumMaterials && i < used.size(); i++) {
            if (!used[i]) {
                continue;
            }
            const aiMaterial& source = *m_scene->mMaterials[i];
            MaterialOutput material;
            material.material.importBlock.sourcePath = m_relativeSourcePath;
            material.material.importBlock.sourceIndex = i;
            material.material.importBlock.sourceName = source.GetName().C_Str();
            material.name = material.material.importBlock.sourceName;
            if (material.name.empty()) {
                material.name = std::string(UNNAMED_MATERIAL) + std::to_string(i);
            }
            readValues(source, material.material);
            planInputs(source, material, out);

            out.materialBySource[i] = static_cast<std::uint32_t>(out.materials.size());
            out.materials.push_back(std::move(material));
        }
    }

    std::error_code MaterialImporter::write(const MaterialPlan& plan, const Side side, std::vector<AssetRecord>& records) const {
        std::vector<bool> written(plan.textures.size(), false);
        std::vector<std::byte> bytes;
        for (std::size_t i = 0; i < plan.textures.size(); i++) {
            const TextureOutput& output = plan.textures[i];
            ByteImage image;
            if (!loadTexture(output, image)) {
                continue;
            }
            Texture texture = output.texture;
            TextureImporter::encode(image, output.purpose, texture);
            image.pixels.clear();

            bytes.clear();
            texture.write(bytes);
            const std::error_code textureError = m_files->write(output.path, bytes);
            if (textureError) {
                return textureError;
            }
            written[i] = true;
            records.push_back(AssetRecord{texture.id, TEXTURE_TYPE_TAG, side, output.path});
        }

        for (const MaterialOutput& output: plan.materials) {
            Material material = output.material;
            const std::array<AssetRef<Texture>*, 4> inputs{&material.baseColorTexture, &material.occlusionRoughnessMetallicTexture, &material.normalTexture, &material.emissiveTexture};
            for (std::size_t input = 0; input < inputs.size(); input++) {
                const std::uint32_t texture = output.textures[input];
                if (texture != NO_PLANNED_OUTPUT && written[texture]) {
                    inputs[input]->id = plan.textures[texture].texture.id;
                }
            }

            bytes.clear();
            material.write(bytes);
            const std::error_code materialError = m_files->write(output.path, bytes);
            if (materialError) {
                return materialError;
            }
            records.push_back(AssetRecord{material.id, MATERIAL_TYPE_TAG, side, output.path});
        }
        return {};
    }

    std::error_code MaterialImporter::decodeImage(const std::span<const std::byte> bytes, ByteImage& out) {
        if (DdsReader::isDds(bytes)) {
            return DdsReader::decode(bytes, out);
        }
        return ImageDecoder::decodeBytes(bytes, out);
    }

    // A value the source does not hold keeps the file's default, except metallic: a source with no metallic factor is
    // not a glTF file, and its surfaces are taken as dielectric.
    void MaterialImporter::readValues(const aiMaterial& source, Material& out) {
        aiColor4D baseColor;
        aiColor3D diffuse;
        if (source.Get(AI_MATKEY_BASE_COLOR, baseColor) == aiReturn_SUCCESS) {
            out.baseColorFactor = Vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
        } else if (source.Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == aiReturn_SUCCESS) {
            float opacity = 1.0f;
            source.Get(AI_MATKEY_OPACITY, opacity);
            out.baseColorFactor = Vec4(diffuse.r, diffuse.g, diffuse.b, opacity);
        }

        out.metallicFactor = 0.0f;
        source.Get(AI_MATKEY_METALLIC_FACTOR, out.metallicFactor);
        source.Get(AI_MATKEY_ROUGHNESS_FACTOR, out.roughnessFactor);
        source.Get(AI_MATKEY_GLTF_TEXTURE_STRENGTH(aiTextureType_LIGHTMAP, 0), out.occlusionStrength);
        source.Get(AI_MATKEY_GLTF_TEXTURE_SCALE(aiTextureType_NORMALS, 0), out.normalScale);

        aiColor3D emissive;
        if (source.Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == aiReturn_SUCCESS) {
            out.emissiveFactor = Vec3(emissive.r, emissive.g, emissive.b);
        }
        float emissiveStrength = 1.0f;
        source.Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveStrength);
        out.emissiveFactor *= emissiveStrength;

        int twoSided = 0;
        source.Get(AI_MATKEY_TWOSIDED, twoSided);
        out.doubleSided = twoSided != 0;

        aiString alphaMode;
        if (source.Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == aiReturn_SUCCESS) {
            const std::string_view mode = alphaMode.C_Str();
            if (mode == GLTF_ALPHA_MASK) {
                out.alphaMode = MaterialAlphaMode::Cutout;
            } else if (mode == GLTF_ALPHA_BLEND) {
                out.alphaMode = MaterialAlphaMode::Blended;
            }
        }
        source.Get(AI_MATKEY_GLTF_ALPHACUTOFF, out.alphaCutoff);

        int shadingModel = 0;
        if (source.Get(AI_MATKEY_SHADING_MODEL, shadingModel) == aiReturn_SUCCESS && shadingModel == aiShadingMode_Unlit) {
            out.shadingModel = MaterialShadingModel::Unlit;
        }
    }

    void MaterialImporter::planInputs(const aiMaterial& source, MaterialOutput& material, MaterialPlan& out) const {
        const SourceImage none;
        SourceImage image;
        TextureSampler sampler;
        if (readImage(source, aiTextureType_BASE_COLOR, image, sampler) || readImage(source, aiTextureType_DIFFUSE, image, sampler)) {
            material.textures[BASE_COLOR_INPUT] = addTexture(TexturePurpose::BaseColor, image, none, sampler, m_relativeSourcePath, out);
        }

        // glTF's occlusion arrives as a lightmap. Another format's lightmap is baked lighting, not occlusion.
        SourceImage occlusion;
        TextureSampler occlusionSampler;
        const int occlusionType = m_gltfSource ? aiTextureType_LIGHTMAP : aiTextureType_AMBIENT_OCCLUSION;
        const bool hasOcclusion = readImage(source, occlusionType, occlusion, occlusionSampler);
        SourceImage metallicRoughness;
        TextureSampler metallicRoughnessSampler;
        const bool hasMetallicRoughness = readMetallicRoughness(source, metallicRoughness, metallicRoughnessSampler);
        if (hasOcclusion || hasMetallicRoughness) {
            if (hasOcclusion && hasMetallicRoughness && occlusionSampler != metallicRoughnessSampler) {
                TE_LOGGER_WARN("Import: material '{0}' samples occlusion and metallic-roughness differently, so the packed texture takes the metallic-roughness sampler", material.name);
            }
            const TextureSampler packedSampler = hasMetallicRoughness ? metallicRoughnessSampler : occlusionSampler;
            material.textures[OCCLUSION_ROUGHNESS_METALLIC_INPUT] = addTexture(TexturePurpose::OcclusionRoughnessMetallic, occlusion, metallicRoughness, packedSampler, m_relativeSourcePath, out);
        }

        if (readImage(source, aiTextureType_NORMALS, image, sampler)) {
            material.textures[NORMAL_INPUT] = addTexture(TexturePurpose::Normal, image, none, sampler, m_relativeSourcePath, out);
        }
        if (readImage(source, aiTextureType_EMISSIVE, image, sampler)) {
            material.textures[EMISSIVE_INPUT] = addTexture(TexturePurpose::Emissive, image, none, sampler, m_relativeSourcePath, out);
        }
    }

    // A wrap or a filter the source leaves out is glTF's default: repeat and linear.
    bool MaterialImporter::readImage(const aiMaterial& source, const int textureType, SourceImage& image, TextureSampler& sampler) const {
        const aiTextureType type = static_cast<aiTextureType>(textureType);
        aiString path;
        if (source.Get(AI_MATKEY_TEXTURE(type, 0), path) != aiReturn_SUCCESS || path.length == 0) {
            return false;
        }

        const std::pair<const aiTexture*, int> embedded = m_scene->GetEmbeddedTextureAndIndex(path.C_Str());
        if (embedded.first != nullptr) {
            image.embeddedIndex = embedded.second;
            image.name = embedded.first->mFilename.length > 0 ? std::string(embedded.first->mFilename.C_Str()) : "*" + std::to_string(embedded.second);
        } else {
            image.embeddedIndex = -1;
            image.name = m_gltfSource ? decodeUri(path.C_Str()) : std::string(path.C_Str());
            std::ranges::replace(image.name, '\\', '/');
        }

        sampler = TextureSampler{};
        int mapMode = 0;
        if (source.Get(AI_MATKEY_MAPPINGMODE_U(type, 0), mapMode) == aiReturn_SUCCESS) {
            sampler.wrapU = toWrap(mapMode);
        }
        if (source.Get(AI_MATKEY_MAPPINGMODE_V(type, 0), mapMode) == aiReturn_SUCCESS) {
            sampler.wrapV = toWrap(mapMode);
        }
        int magnifyFilter = 0;
        if (source.Get(AI_MATKEY_GLTF_MAPPINGFILTER_MAG(type, 0), magnifyFilter) == aiReturn_SUCCESS && magnifyFilter == GLTF_NEAREST_FILTER) {
            sampler.filter = TextureFilter::Nearest;
        }
        return true;
    }

    // glTF's metallic-roughness image holds roughness in green and metallic in blue, and assimp lists it as both the
    // metalness and the roughness texture. Separate grey images from another format are not packed.
    bool MaterialImporter::readMetallicRoughness(const aiMaterial& source, SourceImage& image, TextureSampler& sampler) const {
        SourceImage metalness;
        TextureSampler metalnessSampler;
        const bool hasMetalness = readImage(source, aiTextureType_METALNESS, metalness, metalnessSampler);
        SourceImage roughness;
        TextureSampler roughnessSampler;
        const bool hasRoughness = readImage(source, aiTextureType_DIFFUSE_ROUGHNESS, roughness, roughnessSampler);
        if (!hasMetalness && !hasRoughness) {
            return false;
        }
        if (!hasMetalness || !hasRoughness || metalness != roughness) {
            TE_LOGGER_WARN("Import: material '{0}' has separate metalness and roughness images, which are not imported", source.GetName().C_Str());
            return false;
        }
        image = std::move(metalness);
        sampler = metalnessSampler;
        return true;
    }

    bool MaterialImporter::loadTexture(const TextureOutput& output, ByteImage& out) const {
        const std::string& key = output.texture.importBlock.sourceName;
        const bool packed = output.purpose == TexturePurpose::OcclusionRoughnessMetallic && output.image != output.secondImage;
        if (!packed) {
            const SourceImage& image = output.image.name.empty() ? output.secondImage : output.image;
            const std::error_code loadError = loadImage(image, out);
            if (loadError) {
                TE_LOGGER_WARN("Import: texture '{0}' is left out, because '{1}' could not be read: {2}", key, image.name, loadError.message());
                return false;
            }
            return true;
        }

        std::array<ByteImage, 2> images;
        const std::array<const SourceImage*, 2> sources{&output.image, &output.secondImage};
        for (std::size_t i = 0; i < sources.size(); i++) {
            if (sources[i]->name.empty()) {
                continue;
            }
            const std::error_code loadError = loadImage(*sources[i], images[i]);
            if (loadError) {
                TE_LOGGER_WARN("Import: texture '{0}' is left out, because '{1}' could not be read: {2}", key, sources[i]->name, loadError.message());
                return false;
            }
        }
        TextureImporter::packOcclusionRoughnessMetallic(images[0], images[1], out);
        return true;
    }

    std::error_code MaterialImporter::loadImage(const SourceImage& image, ByteImage& out) const {
        if (image.embeddedIndex >= 0) {
            const aiTexture& texture = *m_scene->mTextures[image.embeddedIndex];
            if (texture.mHeight != 0) {
                TE_LOGGER_WARN("Import: embedded image '{0}' holds raw pixels, which are not imported", image.name);
                return ProjectError::UnsupportedImageType;
            }
            const std::span<const std::byte> bytes(reinterpret_cast<const std::byte*>(texture.pcData), texture.mWidth);
            return decodeImage(bytes, out);
        }

        std::string path;
        if (!imagePath(image.name, path)) {
            return FileError::InvalidPath;
        }
        std::vector<std::byte> bytes;
        const std::error_code readError = m_files->read(path, bytes);
        if (readError) {
            return readError;
        }
        return decodeImage(bytes, out);
    }

    // Joins name to the source's folder, resolving "." and "..". An absolute name, or one that climbs above the mount's
    // root, has no virtual path.
    bool MaterialImporter::imagePath(const std::string_view name, std::string& out) const {
        if (name.empty() || name.front() == '/' || name.find(':') != std::string_view::npos) {
            return false;
        }
        const std::size_t aliasEnd = m_sourcePath.find("://");
        const std::size_t lastSlash = m_sourcePath.find_last_of('/');
        if (aliasEnd == std::string::npos || lastSlash == std::string::npos) {
            return false;
        }
        const std::size_t rootStart = aliasEnd + 3;
        std::vector<std::string_view> segments;
        const std::string_view folder = lastSlash + 1 > rootStart ? std::string_view(m_sourcePath).substr(rootStart, lastSlash - rootStart) : std::string_view{};

        const auto appendSegments = [&segments](const std::string_view path) {
            std::size_t start = 0;
            while (start <= path.size()) {
                const std::size_t end = std::min(path.find('/', start), path.size());
                const std::string_view segment = path.substr(start, end - start);
                if (segment == "..") {
                    if (segments.empty()) {
                        return false;
                    }
                    segments.pop_back();
                } else if (!segment.empty() && segment != ".") {
                    segments.push_back(segment);
                }
                start = end + 1;
            }
            return true;
        };
        if (!appendSegments(folder) || !appendSegments(name) || segments.empty()) {
            return false;
        }

        out = m_sourcePath.substr(0, rootStart);
        for (std::size_t i = 0; i < segments.size(); i++) {
            if (i > 0) {
                out += '/';
            }
            out += segments[i];
        }
        return true;
    }

    // A texture is keyed by its images, purpose and sampler, so the materials that share one find it here.
    std::uint32_t MaterialImporter::addTexture(const TexturePurpose purpose, const SourceImage& image, const SourceImage& secondImage, const TextureSampler& sampler, const std::string_view relativeSourcePath, MaterialPlan& out) {
        const std::string keyName = TextureImporter::keyName(imageKeyName(image, secondImage, purpose), purpose, sampler);
        const auto found = std::ranges::find_if(out.textures, [&keyName](const TextureOutput& texture) {
            return texture.texture.importBlock.sourceName == keyName;
        });
        if (found != out.textures.end()) {
            return static_cast<std::uint32_t>(found - out.textures.begin());
        }

        TextureOutput texture;
        texture.purpose = purpose;
        texture.image = image;
        texture.secondImage = secondImage;
        texture.name = imageStem(image.name.empty() ? secondImage : image) + "_" + std::string(TextureImporter::purposeName(purpose));
        texture.texture.importBlock.sourcePath = std::string(relativeSourcePath);
        texture.texture.importBlock.sourceName = keyName;
        texture.texture.sampler = sampler;
        out.textures.push_back(std::move(texture));
        return static_cast<std::uint32_t>(out.textures.size() - 1);
    }

    // A packed ORM names both images, with an empty side for a missing one, so "ao.png+" and "ao.png" stay two keys.
    std::string MaterialImporter::imageKeyName(const SourceImage& image, const SourceImage& secondImage, const TexturePurpose purpose) {
        if (purpose != TexturePurpose::OcclusionRoughnessMetallic || image == secondImage) {
            return image.name;
        }
        return image.name + "+" + secondImage.name;
    }

    std::string MaterialImporter::imageStem(const SourceImage& image) {
        if (image.embeddedIndex >= 0 && image.name.starts_with('*')) {
            return std::string(UNNAMED_TEXTURE) + std::to_string(image.embeddedIndex);
        }
        std::string_view stem = image.name;
        const std::size_t slash = stem.find_last_of('/');
        if (slash != std::string_view::npos) {
            stem = stem.substr(slash + 1);
        }
        const std::size_t dot = stem.find_last_of('.');
        if (dot != std::string_view::npos && dot > 0) {
            stem = stem.substr(0, dot);
        }
        return std::string(stem);
    }

    // glTF's URIs escape characters such as spaces as %XX. A malformed escape is kept as written.
    std::string MaterialImporter::decodeUri(const std::string_view uri) {
        std::string decoded;
        decoded.reserve(uri.size());
        for (std::size_t i = 0; i < uri.size(); i++) {
            if (uri[i] == '%' && i + 2 < uri.size() && hexDigit(uri[i + 1]) >= 0 && hexDigit(uri[i + 2]) >= 0) {
                decoded.push_back(static_cast<char>(hexDigit(uri[i + 1]) * 16 + hexDigit(uri[i + 2])));
                i += 2;
                continue;
            }
            decoded.push_back(uri[i]);
        }
        return decoded;
    }

    int MaterialImporter::hexDigit(const char character) {
        if (character >= '0' && character <= '9') {
            return character - '0';
        }
        if (character >= 'a' && character <= 'f') {
            return character - 'a' + 10;
        }
        if (character >= 'A' && character <= 'F') {
            return character - 'A' + 10;
        }
        return -1;
    }

    // assimp's decal mode samples nothing past the edge, which clamp is the closest to.
    TextureWrap MaterialImporter::toWrap(const int mapMode) {
        switch (mapMode) {
            case aiTextureMapMode_Clamp:
            case aiTextureMapMode_Decal:
                return TextureWrap::Clamp;
            case aiTextureMapMode_Mirror:
                return TextureWrap::Mirror;
            default:
                return TextureWrap::Repeat;
        }
    }
}
