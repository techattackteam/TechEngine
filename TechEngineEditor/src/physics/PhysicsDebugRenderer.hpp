#pragma once

#include "system/System.hpp"
#include "system/SystemsRegistry.hpp"
#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRenderer.h>


namespace TechEngine {
    class PhysicsDebugRenderer : public JPH::DebugRenderer {
    public:
        JPH_OVERRIDE_NEW_DELETE
        SystemsRegistry& systemsRegistry;

        /// Constructor
        explicit PhysicsDebugRenderer(SystemsRegistry& systemsRegistry);

    protected:
        void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;

        void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override;

    public
    :
        void SetCameraPos(JPH::RVec3Arg inCameraPos) {
            mCameraPos = inCameraPos;
            mCameraPosSet = true;
        }

        void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override {
            DrawLine(inV1, inV2, inColor);
            DrawLine(inV2, inV3, inColor);
            DrawLine(inV3, inV1, inColor);
        }

    protected
    :
        Batch CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) override;

        Batch CreateTriangleBatch(const Vertex* inVertices, int inVertexCount, const JPH::uint32* inIndices, int inIndexCount) override;

        void DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef& inGeometry, ECullMode inCullMode, ECastShadow inCastShadow, EDrawMode inDrawMode) override;

    private
    :
        class BatchImpl : public JPH::RefTargetVirtual {
        public:
            JPH_OVERRIDE_NEW_DELETE

            virtual void AddRef() override {
                ++mRefCount;
            }

            virtual void Release() override {
                if (--mRefCount == 0) delete this;
            }

            JPH::Array<Triangle> mTriangles;

        private:
            JPH::atomic<JPH::uint32> mRefCount = 0;
        };

        JPH::RVec3 mCameraPos;
        bool mCameraPosSet = false;
    };
}
