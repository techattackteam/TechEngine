#pragma once

#include "serialization/StreamReader.hpp"
#include "serialization/StreamWriter.hpp"
#include "TechEngine/core/resources/mesh/Vertex.hpp"
#include "TechEngine/core/resources/IResource.hpp"

namespace TechEngine {
    class CORE_DLL MeshResource : public IResource {
    private:
        friend class MeshLoader;
        friend class Renderer;

        std::vector<Vertex> m_vertices;
        std::vector<int> m_indices;

    public:
        MeshResource(const std::string& name, std::vector<Vertex> vertices, std::vector<int> indices);

        MeshResource(const MeshResource& rhs);

        void setVertices(const std::vector<Vertex>& vector, const std::vector<int>& indices);

        void addVertices(const std::vector<Vertex>& vector, const std::vector<int>& indices);

        const std::vector<Vertex>& getVertices() const;

        const std::vector<int>& getIndices() const;

    private:

        MeshResource();

        static void serialize(StreamWriter* writer, const MeshResource& mesh);

        static std::shared_ptr<MeshResource> deserialize(StreamReader* reader);
    };
}
