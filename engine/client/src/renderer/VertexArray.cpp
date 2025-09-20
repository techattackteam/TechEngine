#include "VertexArray.hpp"
#include "ErrorCatcher.hpp"
#include "Line.hpp"
#include "VertexBuffer.hpp"
#include "resources/mesh/Vertex.hpp"
#include "ui/UIVertex.hpp"

namespace TechEngine {
    void VertexArray::init() {
        glGenVertexArrays(1, &id);
    }


    VertexArray::~VertexArray() {
        glDeleteVertexArrays(1, &id);
    }

    void VertexArray::bind() const {
        glBindVertexArray(this->id);
    }

    void VertexArray::unBind() const {
        glBindVertexArray(0);
    }

    void VertexArray::addNewBufferOldFormat(const VertexBuffer& vertexBuffer) const {
        bind();
        vertexBuffer.bind();
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)nullptr);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);

        vertexBuffer.unBind();
        unBind();
    }

    void VertexArray::addNewBuffer(const VertexBuffer& vertexBuffer) const {
        bind();
        vertexBuffer.bind();
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)nullptr);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(3 * sizeof(float)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(6 * sizeof(float)));

        /*
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(8 * sizeof(float)));
        */

        vertexBuffer.unBind();
        unBind();
    }

    void VertexArray::addNewLinesBuffer(const VertexBuffer& vertexBuffer) const {
        bind();
        vertexBuffer.bind();
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (const void*)nullptr);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (const void*)(3 * sizeof(float)));
        vertexBuffer.unBind();
        unBind();
    }

    void VertexArray::addNewUIBuffer(const VertexBuffer& vertexBuffer) const {
        bind();
        vertexBuffer.bind();
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (const void*)nullptr);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (const void*)(2 * sizeof(float)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (const void*)(6 * sizeof(float)));
        vertexBuffer.unBind();
        unBind();
    }
}
