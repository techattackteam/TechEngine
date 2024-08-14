#include "PhysicsDebugRenderer.hpp"
#include "renderer/Renderer.hpp"
#include <Jolt/Renderer/DebugRenderer.h>

namespace TechEngine {
    PhysicsDebugRenderer::PhysicsDebugRenderer(SystemsRegistry& systemsRegistry) : systemsRegistry(systemsRegistry) {
        Initialize();
    }

    void PhysicsDebugRenderer::DrawLine(JPH::RVec3Arg inStart, JPH::RVec3Arg inEnd, JPH::ColorArg inColor) {
        Renderer& renderer = systemsRegistry.getSystem<Renderer>();

        renderer.createLine(glm::vec3(inStart.GetX(), inStart.GetY(), inStart.GetZ()),
                            glm::vec3(inEnd.GetX(), inEnd.GetY(), inEnd.GetZ()),
                            glm::vec4(inColor.r, inColor.g, inColor.b, inColor.a));
    }

    void PhysicsDebugRenderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) {
        //FUCK THIS
    }

    JPH::DebugRenderer::Batch PhysicsDebugRenderer::CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) {
        BatchImpl* batch = new BatchImpl;
        if (inTriangles == nullptr || inTriangleCount == 0)
            return batch;

        batch->mTriangles.assign(inTriangles, inTriangles + inTriangleCount);
        return batch;
    }

    JPH::DebugRenderer::Batch PhysicsDebugRenderer::CreateTriangleBatch(const Vertex* inVertices, int inVertexCount, const JPH::uint32* inIndices, int inIndexCount) {
        BatchImpl* batch = new BatchImpl;
        if (inVertices == nullptr || inVertexCount == 0 || inIndices == nullptr || inIndexCount == 0)
            return batch;

        // Convert indexed triangle list to triangle list
        batch->mTriangles.resize(inIndexCount / 3);
        for (size_t t = 0; t < batch->mTriangles.size(); ++t) {
            Triangle& triangle = batch->mTriangles[t];
            triangle.mV[0] = inVertices[inIndices[t * 3 + 0]];
            triangle.mV[1] = inVertices[inIndices[t * 3 + 1]];
            triangle.mV[2] = inVertices[inIndices[t * 3 + 2]];
        }

        return batch;
    }

    void PhysicsDebugRenderer::DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef& inGeometry, ECullMode inCullMode, ECastShadow inCastShadow, EDrawMode inDrawMode) {
        // Figure out which LOD to use
        const LOD* lod = inGeometry->mLODs.data();
        if (mCameraPosSet)
            lod = &inGeometry->GetLOD(JPH::Vec3(mCameraPos), inWorldSpaceBounds, inLODScaleSq);

        // Draw the batch
        const BatchImpl* batch = static_cast<const BatchImpl*>(lod->mTriangleBatch.GetPtr());
        for (const Triangle& triangle: batch->mTriangles) {
            JPH::RVec3 v0 = inModelMatrix * JPH::Vec3(triangle.mV[0].mPosition);
            JPH::RVec3 v1 = inModelMatrix * JPH::Vec3(triangle.mV[1].mPosition);
            JPH::RVec3 v2 = inModelMatrix * JPH::Vec3(triangle.mV[2].mPosition);
            JPH::Color color = inModelColor * triangle.mV[0].mColor;

            switch (inDrawMode) {
                case EDrawMode::Wireframe:
                    DrawLine(v0, v1, color);
                    DrawLine(v1, v2, color);
                    DrawLine(v2, v0, color);
                    break;

                case EDrawMode::Solid:
                    DrawTriangle(v0, v1, v2, color, inCastShadow);
                    break;
            }
        }
    }
}
