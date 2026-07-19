#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Shader.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "../GLFW.hpp"
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

    Shader::Shader(const std::string& name, const UUID& resourceUUID) : IGPUResource(resourceUUID), m_id(-1), m_shaderName(name) {
    }

    const std::string& Shader::getName() const{
        return m_shaderName;
    }


    bool Shader::link(std::unordered_map<ShaderType, std::string> sources) {
        m_id = glCreateProgram();
        if (sources.empty()) {
            TE_LOGGER_ERROR("Shader '{0}' has no attached sources to link.", m_shaderName);
            return false;
        }

        std::vector<uint32_t> compiledShaders;
        for (const auto& [type, source]: sources) {
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

        //TE_LOGGER_INFO("Successfully linked shader program '{0}'.", m_shaderName);
        sources.clear();
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

    void Shader::setUniformUInt(const std::string& name, unsigned int value) {
        glUniform1ui(getUniformLocation(name), value);
    }

    void Shader::setUniformDouble(const std::string& name, double value) {
        glUniform1d(getUniformLocation(name), value);
    }

    void Shader::setUniformIVec3(const std::string& name, glm::ivec3 vector) {
        glUniform3iv(getUniformLocation(name), 1, glm::value_ptr(vector));
    }

    void Shader::setUniformIVec4(const std::string& name, glm::ivec4 vector) {
        glUniform4iv(getUniformLocation(name), 1, glm::value_ptr(vector));
    }

    void Shader::setUniformUVec3(const std::string& name, glm::uvec3 vector) {
        glUniform3uiv(getUniformLocation(name), 1, glm::value_ptr(vector));
    }

    void Shader::setUniformUVec4(const std::string& name, glm::uvec4 vector) {
        glUniform4uiv(getUniformLocation(name), 1, glm::value_ptr(vector));
    }

    void Shader::setUniformDVec2(const std::string& name, glm::dvec2 vector) {
        glUniform2dv(getUniformLocation(name), 1, glm::value_ptr(vector));
    }

    void Shader::setUniformDVec3(const std::string& name, glm::dvec3 vector) {
        glUniform3dv(getUniformLocation(name), 1, glm::value_ptr(vector));
    }

    void Shader::setUniformDVec4(const std::string& name, glm::dvec4 vector) {
        glUniform4dv(getUniformLocation(name), 1, glm::value_ptr(vector));
    }

    void Shader::setUniformMatrix2f(const std::string& name, glm::mat2 matrix) {
        glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void Shader::setUniformMatrix3f(const std::string& name, glm::mat3 matrix) {
        glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void Shader::setUniformMatrix2x3f(const std::string& name, glm::mat2x3 matrix) {
        glUniformMatrix2x3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void Shader::setUniformMatrix3x2f(const std::string& name, glm::mat3x2 matrix) {
        glUniformMatrix3x2fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void Shader::setUniformMatrix2x4f(const std::string& name, glm::mat2x4 matrix) {
        glUniformMatrix2x4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void Shader::setUniformMatrix4x2f(const std::string& name, glm::mat4x2 matrix) {
        glUniformMatrix4x2fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void Shader::setUniformMatrix3x4f(const std::string& name, glm::mat3x4 matrix) {
        glUniformMatrix3x4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void Shader::setUniformMatrix4x3f(const std::string& name, glm::mat4x3 matrix) {
        glUniformMatrix4x3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void Shader::setUniformIntArray(const std::string& name, int* values, uint32_t count) {
        glUniform1iv(getUniformLocation(name), count, values);
    }

    void Shader::setUniformFloatArray(const std::string& name, float* values, uint32_t count) {
        glUniform1fv(getUniformLocation(name), count, values);
    }

    void Shader::setUniformVec2Array(const std::string& name, const std::vector<glm::vec2>& values) {
        glUniform2fv(getUniformLocation(name), values.size(), glm::value_ptr(values[0]));
    }

    void Shader::setUniformVec3Array(const std::string& name, const std::vector<glm::vec3>& values) {
        glUniform3fv(getUniformLocation(name), values.size(), glm::value_ptr(values[0]));
    }

    void Shader::setUniformVec4Array(const std::string& name, const std::vector<glm::vec4>& values) {
        glUniform4fv(getUniformLocation(name), values.size(), glm::value_ptr(values[0]));
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
            //TE_LOGGER_WARN("Warning: uniform '{0}' doesn't exist in {1} shader", name, this->m_shaderName);
        } else {
            uniformLocationCache[name] = location;
        }
        return location;
    }
}
