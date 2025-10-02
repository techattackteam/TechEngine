#pragma once
#include <unordered_map>
#include <glm/fwd.hpp>
#include <string>

enum class ShaderType {
    NONE = -1, VERTEX = 0, FRAGMENT = 1, COMPUTE = 2
};

struct ShaderSource {
    std::string vertexShader;
    std::string fragmentShader;
};

namespace TechEngine {
    class Shader {
    private:
        std::string shaderName;
        std::unordered_map<std::string, int32_t> uniformLocationCache;

        uint32_t compileShader(uint32_t type, const std::string& source);

        uint32_t createGraphicsProgram(const std::string& vertexShader, const std::string& fragmentShader);

        uint32_t createComputeProgram(const std::string& computeShader);

        ShaderSource parseShader(const char* vertexShaderPath, const char* fragmentShaderPath);

        std::string parseSingleShader(const std::string& sources);

    public:
        uint32_t id;

        Shader(const std::string& name, const char* vertexShaderPath, const char* fragmentShaderPath);

        Shader(const std::string& name, const char* computeShaderPath);

        void bind() const;

        void unBind() const;

        uint32_t getUniformLocation(const std::string& name);

        void setUniformMatrix4f(const std::string& name, glm::mat4 matrix);

        void setUniformVec2(const std::string& name, glm::vec2 vector);

        void setUniformUVec2(const std::string& name, glm::uvec2 vector);

        void setUniformIVec2(const std::string& name, glm::ivec2 vector);

        void setUniformVec3(const std::string& name, glm::vec3 vector);

        void setUniformVec4(const std::string& name, glm::vec4 vector);

        void setUniformInt(const std::string& name, int value);

        void setUniformBool(const std::string& name, bool value);

        void setUniformFloat(const std::string& name, float value);
    };
}
