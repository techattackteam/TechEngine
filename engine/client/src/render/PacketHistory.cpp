#include <TechEngine/base/diagnostics/Log.hpp>

#include <render/FramePacketPool.hpp>
#include <render/PacketHistory.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <utility>

namespace TechEngine {
    PacketHistory::PacketHistory(FramePacketPool& pool) : m_pool(&pool) {
    }

    PacketHistory::~PacketHistory() {
        m_pool->release(std::move(m_previous));
        m_pool->release(std::move(m_current));
    }

    void PacketHistory::acquire(std::unique_ptr<FramePacket> packet) {
        if (!std::isfinite(packet->fixedDeltaTime) || packet->fixedDeltaTime <= 0.0) {
            TE_LOGGER_ERROR("Rejected frame packet with invalid fixed interval");
            m_pool->release(std::move(packet));
            return;
        }
        if (!m_current || packet->timeline > m_current->timeline) {
            m_pool->release(std::move(m_previous));
            m_pool->release(std::move(m_current));
            m_current = std::move(packet);
            return;
        }
        if (packet->timeline == m_current->timeline && packet->tick == m_current->tick && packet->sequence > m_current->sequence) {
            m_pool->release(std::move(m_current));
            m_current = std::move(packet);
            return;
        }

        if (packet->timeline < m_current->timeline || packet->tick <= m_current->tick) {
            m_pool->release(std::move(packet));
            return;
        }
        if (packet->tickTime <= m_current->tickTime || packet->fixedDeltaTime != m_current->fixedDeltaTime) {
            TE_LOGGER_ERROR("Frame packet timeline changed without a new generation");
            m_pool->release(std::move(m_previous));
            m_pool->release(std::move(m_current));
        } else {
            m_pool->release(std::move(m_previous));
            m_previous = std::move(m_current);
        }
        m_current = std::move(packet);
    }

    double PacketHistory::alpha(Clock::TimePoint now) const {
        if (!m_previous || !m_current) {
            return 1.0;
        }
        const double gap = std::chrono::duration<double>(m_current->tickTime - m_previous->tickTime).count();
        const double elapsed = std::chrono::duration<double>(now - m_previous->tickTime).count() - m_current->fixedDeltaTime;
        return std::clamp(elapsed / gap, 0.0, 1.0);
    }

    void PacketHistory::prepareFrame(Clock::TimePoint now, const InputState& input, FrameView& out) const {
        const auto blend = static_cast<float>(alpha(now));
        out.packet = m_current.get();
        out.clearColor = m_current ? m_current->clearColor : std::array<float, 4>{};
        out.drawTriangle = m_current && m_current->drawTriangle;
        out.input = m_current ? m_current->input : InputState{};
        if (m_previous && m_current) {
            for (std::size_t i = 0; i < out.clearColor.size(); i++) {
                out.clearColor[i] = std::lerp(m_previous->clearColor[i], m_current->clearColor[i], blend);
            }
        }
        interpolateScene(blend, out);
        if (!m_current || input.focusGeneration > out.input.focusGeneration || (input.focusGeneration == out.input.focusGeneration && input.sequence >= out.input.sequence)) {
            out.input = input;
        }
    }

    void PacketHistory::interpolateScene(float blend, FrameView& out) const {
        out.instanceWorlds.clear();
        out.hasCamera = false;
        out.camera = PacketCamera{};

        if (!m_current) {
            return;
        }

        const auto& current = *m_current;
        out.instanceWorlds.resize(current.instances.size());

        if (m_previous) {
            const auto& previous = *m_previous;
            for (std::size_t i = 0; i < current.instances.size(); i++) {
                const auto& instance = current.instances[i];
                const PacketInstance* matched = nullptr;
                if (i < previous.instances.size() && previous.instances[i].entity == instance.entity) {
                    matched = &previous.instances[i];
                } else {
                    const auto found = std::find_if(previous.instances.begin(), previous.instances.end(), [&](const PacketInstance& candidate) {
                        return candidate.entity == instance.entity;
                    });
                    if (found != previous.instances.end()) {
                        matched = &*found;
                    }
                }
                if (matched != nullptr) {
                    for (int column = 0; column < 4; column++) {
                        out.instanceWorlds[i][column] = glm::mix(matched->world[column], instance.world[column], blend);
                    }
                } else {
                    out.instanceWorlds[i] = instance.world;
                }
            }
        } else {
            for (std::size_t i = 0; i < current.instances.size(); i++) {
                out.instanceWorlds[i] = current.instances[i].world;
            }
        }

        if (current.hasCamera) {
            out.hasCamera = true;
            if (m_previous && m_previous->hasCamera) {
                for (int column = 0; column < 4; column++) {
                    out.camera.world[column] = glm::mix(m_previous->camera.world[column], current.camera.world[column], blend);
                }
            } else {
                out.camera.world = current.camera.world;
            }
            out.camera.settings = current.camera.settings;
        }
    }
}
