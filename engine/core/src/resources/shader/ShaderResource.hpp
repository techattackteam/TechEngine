#pragma once
#include "TechEngine/core/resources/IResource.hpp"

namespace TechEngine {
    enum class ShaderType {
        Vertex,
        Fragment,
        Geometry,
        Compute,
        TessellationControl,
        TessellationEvaluation
    };

    class CORE_DLL ShaderResource : public IResource {
    private:
        std::unordered_map<ShaderType, std::string> m_sources;

    public:
        ShaderResource(const std::string& name);

        void setSource(ShaderType shaderType, const std::string& source);

        void removeSource(ShaderType shaderType);

        const std::unordered_map<ShaderType, std::string>& getSources() const;

    private:
        ShaderResource();
    };
}
