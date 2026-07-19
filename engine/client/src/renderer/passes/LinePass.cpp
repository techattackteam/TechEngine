#include "LinePass.hpp"

#include <vector>

#include "renderer/Line.hpp"
#include "renderer/graph/FrameContext.hpp"
#include "renderer/graph/RenderResources.hpp"
#include "renderer/resources/GpuResourceManager.hpp"

namespace TechEngine {
    void LinePass::execute(const FrameContext& frame, RenderResources& resources) {
        frame.gpu->changeActiveShader("line");
        frame.gpu->getActiveShader()->setUniformMatrix4f("projection", frame.projectionMatrix);
        frame.gpu->getActiveShader()->setUniformMatrix4f("view", frame.viewMatrix);

        // This needs to change and only do if the data has changed
        std::vector<LineVertex> vertices;
        for (Line line: m_lines) {
            vertices.push_back(line.getStart());
            vertices.push_back(line.getEnd());
        }
        resources.vertexBuffers[RenderResources::BufferLines].addData(vertices.data(), vertices.size() * sizeof(LineVertex), 0);
        resources.vertexBuffers[RenderResources::BufferLines].bind();
        resources.vertexArrays[RenderResources::BufferLines].bind();

        glDrawArrays(GL_LINES, 0, m_lines.size() * 2);

        resources.vertexBuffers[RenderResources::BufferLines].unBind();
        resources.vertexArrays[RenderResources::BufferLines].unBind();
        m_lines.clear();
    }
}
