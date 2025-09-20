#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Shader.hpp"
#include "GLFW.hpp"
#include "ErrorCatcher.hpp"
#include "core/Logger.hpp"

namespace TechEngine {
    Shader::Shader(const std::string& name, const char* vertexShaderPath, const char* fragmentShaderPath) {
        this->shaderName = name;
        ShaderSource shaderSource = parseShader(vertexShaderPath, fragmentShaderPath);
        id = createShader(shaderSource.vertexShader, shaderSource.fragmentShader);
    }

    uint32_t Shader::createShader(const std::string& vertexShader, const std::string& fragmentShader) {
        uint32_t program;
        program = glCreateProgram();
        uint32_t vs = compileShader(GL_VERTEX_SHADER, vertexShader);
        uint32_t fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);
        if (vs == -1 || fs == -1) {
            return -1;
        }
        glAttachShader(program, vs);
        glAttachShader(program, fs);

        glLinkProgram(program);
        glValidateProgram(program);

        glDeleteShader(vs);
        glDeleteShader(fs);
        return program;
    }

    uint32_t Shader::compileShader(uint32_t type, const std::string& source) {
        uint32_t shaderID;
        shaderID = glCreateShader(type);
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
            TE_LOGGER_ERROR("Failed to compile {0} {1} shader!", (type == GL_VERTEX_SHADER ? "vertices " : "fragment "), shaderName);
            TE_LOGGER_CRITICAL(message);
            glDeleteShader(shaderID);
            delete[] message;
            return -1;
        }

        return shaderID;
    }

    void Shader::bind() const {
        glUseProgram(id);
    }

    void Shader::unBind() const {
        glUseProgram(0);
    }

    ShaderSource Shader::parseShader(const char* vertexShaderPath, const char* fragmentShaderPath) {
        //0 = vertices. 1 = fragment
        std::string shaderCode[2];
        std::ifstream shaderFile[2];
        shaderFile[0].exceptions(std::ifstream::failbit | std::ifstream::badbit);
        shaderFile[1].exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            // open files
            shaderFile[0].open(vertexShaderPath);
            shaderFile[1].open(fragmentShaderPath);
            std::stringstream shaderStream[2];
            // read file's inBuffer contents into streams
            shaderStream[0] << shaderFile[0].rdbuf();
            shaderStream[1] << shaderFile[1].rdbuf();
            // close file handlers
            shaderFile[0].close();
            shaderFile[1].close();
            // convert stream into string
            shaderCode[0] = shaderStream[0].str();
            shaderCode[1] = shaderStream[1].str();
        } catch (std::ifstream::failure& e) {
            TE_LOGGER_CRITICAL("Error while reading shader file at path: {0} or {1}.", vertexShaderPath, fragmentShaderPath);
            TE_LOGGER_CRITICAL(e.what());
        }
        return ShaderSource({shaderCode[0], shaderCode[1]});
    }

    void Shader::setUniformMatrix4f(const std::string& name, glm::mat4 matrix) {
        glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
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

    uint32_t Shader::getUniformLocation(const std::string& name) {
        if (uniformLocationCache.find(name) != uniformLocationCache.end()) {
            return uniformLocationCache[name];
        }
        int32_t location;
        location = glGetUniformLocation(id, name.c_str());
        if (location == -1) {
            std::cout << "Warning: uniform '" << name << "' doesn't exist in " << this->shaderName << " shader" << std::endl;
        } else {
            uniformLocationCache[name] = location;
        }
        return location;
    }
}