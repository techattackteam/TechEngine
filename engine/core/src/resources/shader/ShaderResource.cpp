#include "ShaderResource.hpp"

namespace TechEngine {
    ShaderResource::ShaderResource(const std::string& name) : IResource(name) {
    }


    void ShaderResource::setSource(ShaderType shaderType, const std::string& source) {
        m_sources[shaderType] = source;
    }

    void ShaderResource::removeSource(ShaderType shaderType) {
        m_sources.erase(shaderType);
    }

    const std::unordered_map<ShaderType, std::string>& ShaderResource::getSources() const {
        return m_sources;
    }

    ShaderResource::ShaderResource() {
    }
}
