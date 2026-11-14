#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    void Material::write(std::vector<std::byte>& out) const {
        TE_PROFILER_FUNCTION();
        Writer writer{out};
        writer.writeHeader(MATERIAL_KIND);
        writer.write(id.value());
        importBlock.write(writer);
        writer.write(static_cast<std::uint8_t>(surface));
        writer.write(static_cast<std::uint8_t>(shadingModel));
        writer.write(static_cast<std::uint8_t>(alphaMode));
        writer.write(alphaCutoff);
        writer.write(doubleSided);
        writer.write(baseColorFactor.r);
        writer.write(baseColorFactor.g);
        writer.write(baseColorFactor.b);
        writer.write(baseColorFactor.a);
        writer.write(baseColorTexture);
        writer.write(occlusionStrength);
        writer.write(roughnessFactor);
        writer.write(metallicFactor);
        writer.write(occlusionRoughnessMetallicTexture);
        writer.write(normalScale);
        writer.write(normalTexture);
        writer.write(emissiveFactor.r);
        writer.write(emissiveFactor.g);
        writer.write(emissiveFactor.b);
        writer.write(emissiveTexture);
    }

    std::error_code Material::read(const std::span<const std::byte> bytes) {
        TE_PROFILER_FUNCTION();
        Reader reader{bytes};
        BlobHeader header;
        reader.readHeader(MATERIAL_KIND, header);
        std::uint64_t idValue = 0;
        reader.read(idValue);
        if (!reader.ok()) {
            return reader.error();
        }

        ImportBlock readImportBlock;
        const std::error_code importError = readImportBlock.read(reader);
        if (importError) {
            return importError;
        }

        std::uint8_t surfaceValue = 0;
        std::uint8_t shadingModelValue = 0;
        std::uint8_t alphaModeValue = 0;
        std::uint8_t doubleSidedValue = 0;
        Material readMaterial;
        reader.read(surfaceValue);
        reader.read(shadingModelValue);
        reader.read(alphaModeValue);
        reader.read(readMaterial.alphaCutoff);
        reader.read(doubleSidedValue);
        reader.read(readMaterial.baseColorFactor.r);
        reader.read(readMaterial.baseColorFactor.g);
        reader.read(readMaterial.baseColorFactor.b);
        reader.read(readMaterial.baseColorFactor.a);
        reader.read(readMaterial.baseColorTexture);
        reader.read(readMaterial.occlusionStrength);
        reader.read(readMaterial.roughnessFactor);
        reader.read(readMaterial.metallicFactor);
        reader.read(readMaterial.occlusionRoughnessMetallicTexture);
        reader.read(readMaterial.normalScale);
        reader.read(readMaterial.normalTexture);
        reader.read(readMaterial.emissiveFactor.r);
        reader.read(readMaterial.emissiveFactor.g);
        reader.read(readMaterial.emissiveFactor.b);
        reader.read(readMaterial.emissiveTexture);
        if (!reader.ok()) {
            return reader.error();
        }

        const bool knownSurface = surfaceValue == static_cast<std::uint8_t>(MaterialSurface::Standard) && shadingModelValue <= static_cast<std::uint8_t>(MaterialShadingModel::Unlit);
        const bool knownAlphaState = alphaModeValue <= static_cast<std::uint8_t>(MaterialAlphaMode::Blended) && doubleSidedValue <= 1;
        if (!knownSurface || !knownAlphaState || reader.remaining() != 0) {
            return ReadError::Malformed;
        }

        readMaterial.id = ResourceId{idValue};
        readMaterial.importBlock = std::move(readImportBlock);
        readMaterial.surface = static_cast<MaterialSurface>(surfaceValue);
        readMaterial.shadingModel = static_cast<MaterialShadingModel>(shadingModelValue);
        readMaterial.alphaMode = static_cast<MaterialAlphaMode>(alphaModeValue);
        readMaterial.doubleSided = doubleSidedValue != 0;
        *this = std::move(readMaterial);
        return {};
    }

    std::error_code Material::loadPayload(const std::span<const std::byte> bytes, void* payload, std::vector<ResourceId>& dependencies) {
        Material* material = static_cast<Material*>(payload);
        dependencies.clear();
        const std::error_code readError = material->read(bytes);
        if (readError) {
            return readError;
        }

        const std::array<ResourceId, 4> textures{material->baseColorTexture.id, material->occlusionRoughnessMetallicTexture.id, material->normalTexture.id, material->emissiveTexture.id};
        for (const ResourceId texture: textures) {
            if (texture.valid()) {
                dependencies.push_back(texture);
            }
        }
        return {};
    }
}
