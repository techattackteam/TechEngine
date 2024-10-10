#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRendererSimple.h>


namespace TechEngine {
    class DebugRenderer : public JPH::DebugRendererSimple {
         std::function<void(const glm::vec3&, const glm::vec3&, const glm::vec4&)> renderLineFunction;

    public:
        DebugRenderer();

        void init(const std::function<void(const glm::vec3&, const glm::vec3&, const glm::vec4&)>& renderer);

        void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;

        void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override;

        void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override;
    };
}
