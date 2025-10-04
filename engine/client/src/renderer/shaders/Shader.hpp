#pragma once
#include <unordered_map>
#include <glm/fwd.hpp>
#include <string>
#include <vector>
#include <glm/glm.hpp>


namespace TechEngine {
    enum class ShaderType {
        Vertex,
        Fragment,
        Geometry,
        Compute,
        TessellationControl,
        TessellationEvaluation
    };

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

        // Scalar Uniforms
        void setUniformInt(const std::string& name, int value);

        void setUniformFloat(const std::string& name, float value);

        void setUniformBool(const std::string& name, bool value);

        void setUniformUInt(const std::string& name, unsigned int value);

        void setUniformDouble(const std::string& name, double value);

        // Vector Uniforms
        void setUniformVec2(const std::string& name, glm::vec2 vector);

        void setUniformVec3(const std::string& name, glm::vec3 vector);

        void setUniformVec4(const std::string& name, glm::vec4 vector);

        void setUniformIVec2(const std::string& name, glm::ivec2 vector);

        void setUniformIVec3(const std::string& name, glm::ivec3 vector);

        void setUniformIVec4(const std::string& name, glm::ivec4 vector);

        void setUniformUVec2(const std::string& name, glm::uvec2 vector);

        void setUniformUVec3(const std::string& name, glm::uvec3 vector);

        void setUniformUVec4(const std::string& name, glm::uvec4 vector);

        void setUniformDVec2(const std::string& name, glm::dvec2 vector);

        void setUniformDVec3(const std::string& name, glm::dvec3 vector);

        void setUniformDVec4(const std::string& name, glm::dvec4 vector);

        // Matrix Uniforms
        void setUniformMatrix2f(const std::string& name, glm::mat2 matrix);

        void setUniformMatrix3f(const std::string& name, glm::mat3 matrix);

        void setUniformMatrix4f(const std::string& name, glm::mat4 matrix);

        void setUniformMatrix2x3f(const std::string& name, glm::mat2x3 matrix);

        void setUniformMatrix3x2f(const std::string& name, glm::mat3x2 matrix);

        void setUniformMatrix2x4f(const std::string& name, glm::mat2x4 matrix);

        void setUniformMatrix4x2f(const std::string& name, glm::mat4x2 matrix);

        void setUniformMatrix3x4f(const std::string& name, glm::mat3x4 matrix);

        void setUniformMatrix4x3f(const std::string& name, glm::mat4x3 matrix);

        // Array Uniforms
        void setUniformIntArray(const std::string& name, int* values, uint32_t count);

        void setUniformFloatArray(const std::string& name, float* values, uint32_t count);

        void setUniformVec2Array(const std::string& name, const std::vector<glm::vec2>& values);

        void setUniformVec3Array(const std::string& name, const std::vector<glm::vec3>& values);

        void setUniformVec4Array(const std::string& name, const std::vector<glm::vec4>& values);

    private:
        std::string parseShader(const std::string& source);

        uint32_t compileShader(ShaderType type, const std::string& source);

        uint32_t getUniformLocation(const std::string& name);
    };
}
