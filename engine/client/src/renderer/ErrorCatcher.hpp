#pragma once
#include <string>
#include "GLFW.hpp"
#include "core/Logger.hpp"

inline void APIENTRY DebugMessageCallback(GLenum source,
                                          GLenum type,
                                          GLuint id,
                                          GLenum severity,
                                          GLsizei length,
                                          const GLchar* message,
                                          const void* userParam) {
    // ignore non-significant error/warning codes
    // You can customize this to filter out messages you don't care about
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
    std::string msg;
    msg += "---------------\n";
    msg += "Debug message (" + std::to_string(id) += "): " + std::string(message) + "\n";

    switch (source) {
        case GL_DEBUG_SOURCE_API:
            msg += "Source: API\n";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            msg += "Source: Window System\n";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            msg += "Source: Shader Compiler\n";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            msg += "Source: Third Party\n";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            msg += "Source: Application\n";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            msg += "Source: Other\n";
            break;
    }

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            msg += "Type: Error\n";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            msg += "Type: Deprecated Behaviour\n";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            msg += "Type: Undefined Behaviour\n";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            msg += "Type: Portability\n";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            msg += "Type: Performance\n";
            break;
        case GL_DEBUG_TYPE_MARKER:
            msg += "Type: Marker\n";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            msg += "Type: Push Group\n";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            msg += "Type: Pop Group\n";
            break;
        case GL_DEBUG_TYPE_OTHER:
            msg += "Type: Other\n";
            break;
    }

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            msg += "Severity: high\n";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            msg += "Severity: medium\n";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            msg += "Severity: low\n";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            msg += "Severity: notification\n";
            break;
    }
    msg += "---------------\n";
    //TE_LOGGER_CRITICAL(msg);
    //__debugbreak();
}
