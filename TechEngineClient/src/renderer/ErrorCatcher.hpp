#include <iostream>

#pragma once
#define GlCall(x) GlClearError(); x; GlLogCall()

inline void GlClearError() {
    while (glGetError() != GL_NO_ERROR);
}

inline bool GlLogCall() {
    while (GLenum error = glGetError()) {
        std::cout << "OpenGL Error: " << error << std::endl;
        return false;
    }
    return true;
}