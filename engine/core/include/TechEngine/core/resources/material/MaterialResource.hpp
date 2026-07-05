#pragma once

#include "TechEngine/core/resources/IResource.hpp"
#include "serialization/StreamReader.hpp"
#include "serialization/StreamWriter.hpp"

namespace TechEngine {
    class CORE_DLL MaterialResource : public IResource {
    private:
        friend class MaterialLoader;
        friend class MeshLoader;
        friend class ModelLoader;
        friend class AssimpLoader;

        glm::vec4 albedo = glm::vec4(1.0f);
        float metallic = 0;
        float roughness = 0;
        float ambientOcclusion = 0;
        glm::vec4 emission = glm::vec4(1.0f);

        UUID albedoMapUUID = -1;
        UUID normalMapUUID = -1;
        UUID metallicMapUUID = -1;
        UUID roughnessMapUUID = -1;
        UUID ambientOcclusionMapUUID = -1;
        UUID emissionMapUUID = -1;

    public:
        MaterialResource(const std::string& name);

        std::string& getName();

        //TODO: Rethink if this getters and setters are really necessary. This reeks to java programming

        glm::vec4& getAlbedo();

        void setAlbedo(const glm::vec4& albedo);

        float getMetallic() const;

        void setMetallic(float metallic);

        float getRoughness() const;

        void setRoughness(float roughness);

        float getAmbientOcclusion() const;

        void setAmbientOcclusion(float ambientOcclusion);

        glm::vec4& getEmission();

        void setEmission(const glm::vec4& emission);

        const UUID& getAlbedoMapUUID() const;

        void setAlbedoMapID(const UUID& id);

        const UUID& getNormalMapUUID() const;

        void setNormalMapID(const UUID& id);

        const UUID& getMetallicMapUUID() const;

        void setMetallicMapID(const UUID& id);

        const UUID& getRoughnessMapUUID() const;

        void setRoughnessMapID(const UUID& id);

        const UUID& getAmbientOcclusionMapUUID() const;

        void setAmbientOcclusionMapID(const UUID& id);

        const UUID& getEmissionMapUUID() const;

        void setEmissionMapID(const UUID& id);

    private:
        MaterialResource(); // Initilize without creating a new UUID

        static void serialize(StreamWriter* writer, const MaterialResource& material);

        static std::shared_ptr<MaterialResource> deserialize(StreamReader* reader);
    };
}
