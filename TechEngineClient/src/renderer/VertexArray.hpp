#include "VertexBuffer.hpp"

#pragma once

class VertexArray {
private:
    uint32_t id;
public:
    VertexArray() = default;

    ~VertexArray();

    void init(uint32_t id);

    void addNewBuffer(const VertexBuffer &vertexBuffer) const;

    void bind() const;

    void unBind() const;


};