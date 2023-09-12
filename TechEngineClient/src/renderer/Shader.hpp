
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>
#include <string>

#pragma once
enum class ShaderType {
    NONE = -1, VERTEX = 0, FRAGMENT = 1
};

struct ShaderSource {
    std::string vertexShader;
    std::string fragmentShader;
};

class Shader {
private:
    uint32_t id;
    std::string shaderName;
    std::unordered_map<std::string, int32_t> uniformLocationCache;

    uint32_t compileShader(uint32_t type, const std::string &source);

    uint32_t createShader(const std::string &vertexShader, const std::string &fragmentShader);

public:
    Shader(const std::string &name, const char *vertexShaderPath, const char *fragmentShaderPath);

    ShaderSource parseShader(const char *vertexShaderPath, const char *fragmentShaderPath);

    void bind() const;

    void unBind() const;

    uint32_t getUniformLocation(const std::string &name);

    void setUniformMatrix4f(const std::string &name, glm::mat4 matrix);

    void setUniformVec3(const std::string &name, glm::vec3 vector);

    void setUniformVec4(const std::string &name, glm::vec4 vector);

    void setUniformInt(const std::string &name, int value);

    void setUniformBool(const std::string &name, bool value);

    void setUniformFloat(const std::string &name, float value);

};

