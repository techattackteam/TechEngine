#include "VertexArray.hpp"
#include "ErrorCatcher.hpp"
#include "Line.hpp"

namespace TechEngine {

    void VertexArray::init() {
        GlCall(glGenVertexArrays(1, &id));
    }


    VertexArray::~VertexArray() {
        GlCall(glDeleteVertexArrays(1, &id));
    }

    void VertexArray::bind() const {
        GlCall(glBindVertexArray(this->id));
    }

    void VertexArray::unBind() const {
        GlCall(glBindVertexArray(0));
    }

    void VertexArray::addNewBuffer(const VertexBuffer &vertexBuffer) const {
        bind();
        vertexBuffer.bind();
        GlCall(glEnableVertexAttribArray(0));
        GlCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *) nullptr));

        GlCall(glEnableVertexAttribArray(1));
        GlCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *) (3 * sizeof(float))));

        GlCall(glEnableVertexAttribArray(2));
        GlCall(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *) (6 * sizeof(float))));

        GlCall(glEnableVertexAttribArray(3));
        GlCall(glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *) (8 * sizeof(float))));

        vertexBuffer.unBind();
        unBind();
    }

    void VertexArray::addNewLinesBuffer(const VertexBuffer &vertexBuffer) const {
        bind();
        vertexBuffer.bind();
        GlCall(glEnableVertexAttribArray(0));
        GlCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (const void *) nullptr));

        GlCall(glEnableVertexAttribArray(1));
        GlCall(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (const void *) (3 * sizeof(float))));
        vertexBuffer.unBind();
        unBind();
    }
}

