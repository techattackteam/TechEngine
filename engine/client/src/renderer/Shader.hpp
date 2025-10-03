#pragma once
#include <unordered_map>
#include <glm/fwd.hpp>
#include <string>


namespace TechEngine {
    enum class ShaderType {
        Vertex,
        Fragment,
        Geometry,
        Compute,
        TessellationControl,
        TessellationEvaluation
    };

    /*struct ShaderSource {
        std::string vertexShader;
        std::string fragmentShader;
    };*/

    class Shader {
    private:
        uint32_t m_id;
        std::string m_shaderName;
        std::unordered_map<std::string, int32_t> uniformLocationCache;

        std::unordered_map<ShaderType, std::string> m_sources;

    public:
        Shader(const std::string& name);

        void attachSourceFile(ShaderType shaderType, const std::string& path);

        bool link();

        void bind() const;

        void unBind() const;

        void setUniformMatrix4f(const std::string& name, glm::mat4 matrix);

        void setUniformVec2(const std::string& name, glm::vec2 vector);

        void setUniformUVec2(const std::string& name, glm::uvec2 vector);

        void setUniformIVec2(const std::string& name, glm::ivec2 vector);

        void setUniformVec3(const std::string& name, glm::vec3 vector);

        void setUniformVec4(const std::string& name, glm::vec4 vector);

        void setUniformInt(const std::string& name, int value);

        void setUniformBool(const std::string& name, bool value);

        void setUniformFloat(const std::string& name, float value);

    private:
        std::string parseShader(const std::string& source);

        uint32_t compileShader(ShaderType type, const std::string& source);

        uint32_t getUniformLocation(const std::string& name);
    };
}
