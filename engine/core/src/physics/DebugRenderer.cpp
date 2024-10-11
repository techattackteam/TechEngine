#include "DebugRenderer.hpp"

#include "core/Logger.hpp"

namespace TechEngine {
    DebugRenderer::DebugRenderer() : DebugRendererSimple() {
    }

    void DebugRenderer::init(const std::function<void(const glm::vec3&, const glm::vec3&, const glm::vec4&)>& renderer) {
        renderLineFunction = renderer;
    }


    void DebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) {
        float offset = 0.005f;
        glm::vec3 from = glm::vec3(inFrom.GetX() > 0 ? inFrom.GetX() + offset : inFrom.GetX() - offset,
                                   inFrom.GetY() > 0 ? inFrom.GetY() + offset : inFrom.GetY() - offset,
                                   inFrom.GetZ() > 0 ? inFrom.GetZ() + offset : inFrom.GetZ() - offset);
        glm::vec3 to = glm::vec3(inTo.GetX() > 0 ? inTo.GetX() + offset : inTo.GetX() - offset,
                                 inTo.GetY() > 0 ? inTo.GetY() + offset : inTo.GetY() - offset,
                                 inTo.GetZ() > 0 ? inTo.GetZ() + offset : inTo.GetZ() - offset);
        glm::vec4 color = glm::vec4(1.0f, 0, 0, 0.3f);
        renderLineFunction(from, to, color);
    }

    void DebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) {
        DebugRendererSimple::DrawTriangle(inV1, inV2, inV3, inColor, inCastShadow);
        //TE_LOGGER_INFO("DrawTriangle");
    }

    void DebugRenderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) {
        TE_LOGGER_INFO("DrawText3D");
    }
}
