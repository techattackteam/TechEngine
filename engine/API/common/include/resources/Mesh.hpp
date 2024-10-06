#pragma once
#include <string>
#include <vector>

#include "resources/Vertex.hpp"

namespace TechEngine {
    class ResourcesManager;
}

namespace TechEngineAPI {
    class API_DLL Mesh {
    private:
        friend class Resources;

        std::string m_name;
        TechEngine::ResourcesManager* m_resourcesManager;

        Mesh() = default;

    public:
        Mesh(const std::string& name, TechEngine::ResourcesManager* resourcesManager);

        void setVertices(const std::vector<Vertex>& vertices, const std::vector<int>& indices);

        void addVertices(const std::vector<Vertex>& vertices, const std::vector<int>& indices);

        [[nodiscard]] std::string getName();

        void setName(const std::string& name);
    };
}
