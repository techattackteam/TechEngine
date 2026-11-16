#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/client/render/FrameExtraction.hpp>
#include <TechEngine/client/render/FramePacket.hpp>
#include <TechEngine/clientapp/ClientApp.hpp>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace TechEngine {
    ClientApp::ClientApp(const Role role, std::string title) : App(role), m_title(std::move(title)) {
    }

    void ClientApp::startClient(ImGuiHook* imguiHook) {
        const std::error_code startError = m_client.start(
            m_engine,
            m_input,
            1280,
            720,
            m_title,
            [this] {
                requestStop();
            },
            imguiHook);
        if (startError) {
            throw std::runtime_error{"Failed to start the client session: " + startError.message()};
        }
    }

    void ClientApp::stopClient() {
        m_client.stop();
    }

    GLFWwindow* ClientApp::nativeWindow() const {
        return m_client.nativeWindow();
    }

    void ClientApp::detachInput() {
        m_client.detachInput();
    }

    void ClientApp::cancelClose() {
        m_client.cancelClose();
    }

    void ClientApp::setTitle(std::string title) {
        m_title = std::move(title);
        applyTitle();
    }

    void ClientApp::publishFrame(const Scene& scene, const SimulationContext& simulation) {
        std::unique_ptr<FramePacket> packet = m_client.acquirePacket();
        if (packet == nullptr) {
            return;
        }
        packet->clearColor = {0.1F, 0.1F, 0.1F, 1.0F};
        packet->drawTriangle = true;
        packet->tick = simulation.tick;
        packet->tickTime = simulation.tickTime;
        packet->fixedDeltaTime = simulation.fixedDeltaTime;
        packet->timeline = simulation.timeline;
        packet->input = simulation.input.held;
        packet->sequence = m_packetSequence++;
        FrameExtraction::extractFramePacket(scene, m_warnedNoCamera, *packet);
        m_client.publish(std::move(packet));
    }

    void ClientApp::mainThreadUpdate() {
        if (stopRequested()) {
            return;
        }
        if (m_pollNextUpdate) {
            m_pollNextUpdate = false;
            TE_PROFILER_SCOPE("Main.PollEvents");
            m_client.pollEvents();
        } else {
            TE_PROFILER_SCOPE("Main.WaitEvents");
            m_client.waitEvents();
        }
        applyTitle();
        if (m_client.failed()) {
            requestStop();
        }
    }

    void ClientApp::requestMainFrame() {
        m_pollNextUpdate = true;
    }

    void ClientApp::wakeMainThread() {
        m_client.wakeMain();
    }

    bool ClientApp::renderTiming(RenderTiming& out) const {
        return m_client.renderTiming(out);
    }

    void ClientApp::shutdown() {
        stopClient();
        if (m_client.failed()) {
            throw std::runtime_error{"Render thread failed"};
        }
    }

    bool ClientApp::shouldClose() const {
        return m_client.shouldClose();
    }

    void ClientApp::applyTitle() {
        const auto metrics = timingMetrics();
        const auto fps = static_cast<std::uint64_t>(metrics.renderingActive ? metrics.render.framesPerSecond : 0.0);
        const auto tps = static_cast<std::uint64_t>(metrics.simulation.ticksPerSecond);
        std::string title = m_title + " | FPS: " + std::to_string(fps) + " | TPS: " + std::to_string(tps);
        if (title != m_appliedTitle) {
            m_client.setTitle(title);
            m_appliedTitle = std::move(title);
        }
    }
}
