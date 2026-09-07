#include <render/Buffer.hpp>

#include <glad/gl.h>

namespace TechEngine {
    Buffer::~Buffer() {
        shutdown();
    }

    bool Buffer::initialize(const std::span<const std::byte> data) {
        if (m_id != 0 || data.empty()) {
            return false;
        }

        glCreateBuffers(1, &m_id);
        if (m_id == 0) {
            return false;
        }

        glNamedBufferData(m_id, static_cast<std::ptrdiff_t>(data.size()), data.data(), GL_STATIC_DRAW);
        return true;
    }

    unsigned int Buffer::id() const {
        return m_id;
    }

    void Buffer::shutdown() {
        if (m_id == 0) {
            return;
        }

        glDeleteBuffers(1, &m_id);
        m_id = 0;
    }
}
