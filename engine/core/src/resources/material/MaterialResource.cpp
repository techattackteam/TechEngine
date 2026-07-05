#include "TechEngine/core/resources/material/MaterialResource.hpp"


namespace TechEngine {
    MaterialResource::MaterialResource(const std::string& name) : IResource(name) {
    }

    std::string& MaterialResource::getName() {
        return m_name;
    }

    glm::vec4& MaterialResource::getAlbedo() {
        return albedo;
    }

    void MaterialResource::setAlbedo(const glm::vec4& albedo) {
        this->albedo = albedo;
    }

    float MaterialResource::getMetallic() const {
        return metallic;
    }

    void MaterialResource::setMetallic(float metallic) {
        this->metallic = metallic;
    }

    float MaterialResource::getRoughness() const {
        return roughness;
    }

    void MaterialResource::setRoughness(float roughness) {
        this->roughness = roughness;
    }

    float MaterialResource::getAmbientOcclusion() const {
        return ambientOcclusion;
    }

    void MaterialResource::setAmbientOcclusion(float ambientOcclusion) {
        this->ambientOcclusion = ambientOcclusion;
    }

    glm::vec4& MaterialResource::getEmission() {
        return emission;
    }

    void MaterialResource::setEmission(const glm::vec4& emission) {
        this->emission = emission;
    }

    const UUID& MaterialResource::getAlbedoMapUUID() const {
        return albedoMapUUID;
    }

    void MaterialResource::setAlbedoMapID(const UUID& id) {
        albedoMapUUID = id;
    }

    const UUID& MaterialResource::getNormalMapUUID() const {
        return normalMapUUID;
    }

    void MaterialResource::setNormalMapID(const UUID& id) {
        normalMapUUID = id;
    }

    const UUID& MaterialResource::getMetallicMapUUID() const {
        return metallicMapUUID;
    }

    void MaterialResource::setMetallicMapID(const UUID& id) {
        metallicMapUUID = id;
    }

    const UUID& MaterialResource::getRoughnessMapUUID() const {
        return roughnessMapUUID;
    }

    void MaterialResource::setRoughnessMapID(const UUID& id) {
        roughnessMapUUID = id;
    }

    const UUID& MaterialResource::getAmbientOcclusionMapUUID() const {
        return ambientOcclusionMapUUID;
    }

    void MaterialResource::setAmbientOcclusionMapID(const UUID& id) {
        ambientOcclusionMapUUID = id;
    }

    const UUID& MaterialResource::getEmissionMapUUID() const {
        return emissionMapUUID;
    }

    void MaterialResource::setEmissionMapID(const UUID& id) {
        emissionMapUUID = id;
    }

    MaterialResource::MaterialResource() : IResource() {
    }

    void MaterialResource::serialize(StreamWriter* writer, const MaterialResource& material) {
        IResource::serialize(writer, material);
        writer->writeString(material.m_name);
        writer->writeRaw(material.albedo);
        writer->writeRaw(material.metallic);
        writer->writeRaw(material.roughness);
        writer->writeRaw(material.ambientOcclusion);
        writer->writeRaw(material.emission);
        writer->writeRaw(material.albedoMapUUID);
        writer->writeRaw(material.normalMapUUID);
        writer->writeRaw(material.metallicMapUUID);
        writer->writeRaw(material.roughnessMapUUID);
        writer->writeRaw(material.ambientOcclusionMapUUID);
        writer->writeRaw(material.emissionMapUUID);
    }

    std::shared_ptr<MaterialResource> MaterialResource::deserialize(StreamReader* reader) {
        std::shared_ptr<MaterialResource> material = std::shared_ptr<MaterialResource>(new MaterialResource());
        IResource::deserialize(reader, *material);
        reader->readString(material->m_name);
        reader->readRaw(material->albedo);
        reader->readRaw(material->metallic);
        reader->readRaw(material->roughness);
        reader->readRaw(material->ambientOcclusion);
        reader->readRaw(material->emission);
        reader->readRaw(material->albedoMapUUID);
        reader->readRaw(material->normalMapUUID);
        reader->readRaw(material->metallicMapUUID);
        reader->readRaw(material->roughnessMapUUID);
        reader->readRaw(material->ambientOcclusionMapUUID);
        reader->readRaw(material->emissionMapUUID);
        return material;
    }
}
