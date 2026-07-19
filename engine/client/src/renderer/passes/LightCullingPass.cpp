#include "LightCullingPass.hpp"

#include "renderer/graph/FrameContext.hpp"
#include "renderer/graph/RenderResources.hpp"
#include "renderer/resources/GpuResourceManager.hpp"
#include "renderer/shaders/Shader.hpp"
#include "scene/SceneInternal.hpp"
#include "scene/SceneManager.hpp"
#include "systems/SystemsRegistry.hpp"
#include "TechEngine/core/components/Components.hpp"
#include "TechEngine/core/scene/Scene.hpp"

namespace TechEngine {
    bool LightCullingPass::isEnabled(const FrameContext& frame) const {
        return frame.renderMask & RenderMasks::SCENE;
    }

    void LightCullingPass::execute(const FrameContext& frame, RenderResources& resources) {
        frame.gpu->changeActiveShader("createClusters");
        Shader* createClusters = frame.gpu->getActiveShader();
        createClusters->setUniformMatrix4f("u_inverseProjection", glm::inverse(frame.projectionMatrix));
        createClusters->setUniformUVec2("u_screenSize", frame.viewportSize);
        createClusters->setUniformFloat("u_nearPlane", frame.nearPlane);
        createClusters->setUniformFloat("u_farPlane", frame.farPlane);
        createClusters->setUniformUVec3("u_gridSize", frame.gridSize);

        resources.clusterAABBsBuffer.setBindingPoint(0);
        resources.clusterAABBsBuffer.bind();

        glDispatchCompute(frame.gridSize.x, frame.gridSize.y, frame.gridSize.z);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        frame.gpu->changeActiveShader("lightCulling");
        Shader* cullShader = frame.gpu->getActiveShader();
        cullShader->bind();

        cullShader->setUniformMatrix4f("u_view", frame.viewMatrix);
        Scene& scene = frame.systems->getSystem<SceneManager>().getActiveScene();
        uint32_t lightCount = scene.getInternal()->getComponentCount<PointLight>();
        cullShader->setUniformUInt("u_lightCount", lightCount);

        resources.lightsBuffer.setBindingPoint(0);
        resources.lightsIndexBuffer.setBindingPoint(1);
        resources.globalIndexCount.setBindingPoint(2);
        resources.tileInfoBuffer.setBindingPoint(3);
        resources.clusterAABBsBuffer.setBindingPoint(4);

        glDispatchCompute(1, 1, 6);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
}
