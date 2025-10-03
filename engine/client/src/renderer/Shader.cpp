#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Shader.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "GLFW.hpp"
#include "ErrorCatcher.hpp"
#include "core/Logger.hpp"

namespace TechEngine {
    static GLenum ShaderTypeToGL(ShaderType type) {
        switch (type) {
            case ShaderType::Vertex: return GL_VERTEX_SHADER;
            case ShaderType::Fragment: return GL_FRAGMENT_SHADER;
            case ShaderType::Geometry: return GL_GEOMETRY_SHADER;
            case ShaderType::Compute: return GL_COMPUTE_SHADER;
            case ShaderType::TessellationControl: return GL_TESS_CONTROL_SHADER;
            case ShaderType::TessellationEvaluation: return GL_TESS_EVALUATION_SHADER;
        }
        return 0;
    }

    Shader::Shader(const std::string& name) : m_id(-1), m_shaderName(name) {
    }

    void Shader::attachSourceFile(ShaderType shaderType, const std::string& path) {
        m_sources[shaderType] = parseShader(path);
    }

    bool Shader::link() {
        m_id = glCreateProgram();
        if (m_sources.empty()) {
            TE_LOGGER_ERROR("Shader '{0}' has no attached sources to link.", m_shaderName);
            return false;
        }

        std::vector<uint32_t> compiledShaders;
        for (const auto& [type, source]: m_sources) {
            uint32_t shaderId = compileShader(type, source);
            if (shaderId == -1) {
                for (uint32_t id: compiledShaders) {
                    glDeleteShader(id);
                }
                glDeleteProgram(m_id);
                m_id = 0;
                return false;
            }
            glAttachShader(m_id, shaderId);
            compiledShaders.push_back(shaderId);
        }

        glLinkProgram(m_id);

        int result;
        glGetProgramiv(m_id, GL_LINK_STATUS, &result);
        if (result == GL_FALSE) {
            int length;
            glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);
            char* message = new char[length];
            glGetProgramInfoLog(m_id, length, &length, message);
            TE_LOGGER_ERROR("Failed to link shader program '{0}': {1}", m_shaderName, message);
            delete[] message;

            for (uint32_t id: compiledShaders) {
                glDeleteShader(id);
            }
            glDeleteProgram(m_id);
            m_id = 0;
            return false;
        }

        glValidateProgram(m_id);

        for (uint32_t id: compiledShaders) {
            glDetachShader(m_id, id);
            glDeleteShader(id);
        }

        TE_LOGGER_INFO("Successfully linked shader program '{0}'.", m_shaderName);
        m_sources.clear();
        return true;
    }

    void Shader::bind() const {
        glUseProgram(m_id);
    }

    void Shader::unBind() const {
        glUseProgram(0);
    }

    void Shader::setUniformMatrix4f(const std::string& name, glm::mat4 matrix) {
        glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void Shader::setUniformVec2(const std::string& name, glm::vec2 vector) {
        glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(vector));
    }

    void Shader::setUniformUVec2(const std::string& name, glm::uvec2 vector) {
        glUniform2uiv(getUniformLocation(name), 1, glm::value_ptr(vector));
    }

    void Shader::setUniformIVec2(const std::string& name, glm::ivec2 vector) {
        glUniform2iv(getUniformLocation(name), 1, glm::value_ptr(vector));
    }

    void Shader::setUniformVec3(const std::string& name, glm::vec3 vector) {
        glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(vector));
    }

    void Shader::setUniformVec4(const std::string& name, glm::vec4 vector) {
        glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(vector));
    }

    void Shader::setUniformInt(const std::string& name, int value) {
        glUniform1i(getUniformLocation(name), value);
    }

    void Shader::setUniformBool(const std::string& name, bool value) {
        glUniform1i(getUniformLocation(name), value);
    }


    void Shader::setUniformFloat(const std::string& name, float value) {
        glUniform1f(getUniformLocation(name), value);
    }

    std::string Shader::parseShader(const std::string& source) {
        std::string shaderCode;
        std::ifstream shaderFile;
        shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            // open files
            shaderFile.open(source);
            std::stringstream shaderStream;
            // read file's inBuffer contents into streams
            shaderStream << shaderFile.rdbuf();
            // close file handlers
            shaderFile.close();
            // convert stream into string
            shaderCode = shaderStream.str();
        } catch (std::ifstream::failure& e) {
            TE_LOGGER_CRITICAL("Error while reading shader file at path: {0}.", source);
            TE_LOGGER_CRITICAL(e.what());
        }
        return shaderCode;
    }


    uint32_t Shader::compileShader(ShaderType type, const std::string& source) {
        GLenum glType = ShaderTypeToGL(type);
        uint32_t shaderID;
        shaderID = glCreateShader(glType);
        const char* src = source.c_str();
        glShaderSource(shaderID, 1, &src, nullptr);
        glCompileShader(shaderID);

        int result;
        glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
        if (result == GL_FALSE) {
            int length;
            glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &length);
            char* message = new char[length];
            glGetShaderInfoLog(shaderID, length, &length, message);
            TE_LOGGER_CRITICAL("Failed to compile {0} shader for {1}.\n{2}",
                               (type == ShaderType::Vertex ? "vertex" :
                                   type == ShaderType::Fragment ? "fragment" :
                                   type == ShaderType::Geometry ? "geometry" :
                                   type == ShaderType::Compute ? "compute" :
                                   type == ShaderType::TessellationControl ? "tessellation control" :
                                   type == ShaderType::TessellationEvaluation ? "tessellation evaluation" :
                                   "unknown"), this->m_shaderName,
                               message);
            TE_LOGGER_CRITICAL(message);
            glDeleteShader(shaderID);
            delete[] message;
            return -1;
        }

        return shaderID;
    }

    uint32_t Shader::getUniformLocation(const std::string& name) {
        if (uniformLocationCache.find(name) != uniformLocationCache.end()) {
            return uniformLocationCache[name];
        }
        int32_t location;
        location = glGetUniformLocation(m_id, name.c_str());
        if (location == -1) {
            TE_LOGGER_WARN("Warning: uniform '{0}' doesn't exist in {1} shader", name, this->m_shaderName);
        } else {
            uniformLocationCache[name] = location;
        }
        return location;
    }
}
