#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneCommandBuffer.hpp>
#include <TechEngine/core/systems/ISystem.hpp>
#include <TechEngine/core/systems/InputNotification.hpp>
#include <TechEngine/core/systems/ScheduleAccess.hpp>
#include <TechEngine/core/systems/SerialExecutor.hpp>
#include <TechEngine/core/systems/TaskGraph.hpp>

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace TechEngine {
    class SerialExecutor::Impl {
    public:
        struct Node {
            ScheduleAccess access;
            ISystem* system = nullptr;
            SceneCommandBuffer commands;
            std::vector<TaskGraphEventHandler> eventHandlers;
            std::vector<InputHandler> inputHandlers;
        };

        using Level = std::vector<Node>;

        std::vector<Level> levels;
        std::vector<Entity> spawned;
        std::vector<InputNotification> holds;

        InputNotification toNotification(const InputEvent& event);

        void collectHolds(const InputState& held);
    };

    SerialExecutor::SerialExecutor(const TaskGraph& graph) : m_impl(std::make_unique<Impl>()) {
        for (const TaskGraphLevel& sourceLevel: graph.getLevels()) {
            Impl::Level level;
            level.reserve(sourceLevel.size());
            for (const TaskGraphNode& sourceNode: sourceLevel) {
                level.push_back({sourceNode.access, sourceNode.system, {}, sourceNode.eventHandlers, sourceNode.inputHandlers});
            }
            m_impl->levels.push_back(std::move(level));
        }
    }

    InputNotification SerialExecutor::Impl::toNotification(const InputEvent& event) {
        InputNotification notification;
        notification.sequence = event.sequence;
        switch (event.kind) {
            case InputKind::Key:
                notification.kind = InputNotificationKind::Key;
                notification.key = event.key;
                notification.pressed = event.pressed;
                break;
            case InputKind::Button:
                notification.kind = InputNotificationKind::Button;
                notification.button = event.button;
                notification.pressed = event.pressed;
                break;
            case InputKind::Motion:
                notification.kind = InputNotificationKind::Motion;
                notification.x = event.x;
                notification.y = event.y;
                break;
            case InputKind::Focus:
                notification.kind = InputNotificationKind::Focus;
                notification.pressed = event.pressed;
                break;
        }
        return notification;
    }

    void SerialExecutor::Impl::collectHolds(const InputState& held) {
        holds.clear();
        for (std::size_t i = 0; i < KEY_COUNT; i++) {
            if (held.keys.test(i)) {
                InputNotification notification;
                notification.kind = InputNotificationKind::KeyHold;
                notification.key = static_cast<Key>(i);
                holds.push_back(notification);
            }
        }
        for (std::size_t i = 0; i < MOUSE_BUTTON_COUNT; i++) {
            if (held.buttons.test(i)) {
                InputNotification notification;
                notification.kind = InputNotificationKind::ButtonHold;
                notification.button = static_cast<MouseButton>(i);
                holds.push_back(notification);
            }
        }
    }

    SerialExecutor::~SerialExecutor() = default;

    void SerialExecutor::execute(Scene& scene, const SimulationContext& context, TickBarrierServices& barrier) {
        TE_PROFILER_FUNCTION();
        m_impl->collectHolds(context.input.held);
        try {
            {
                TE_PROFILER_SCOPE("SerialExecutor.Systems");
                for (Impl::Level& level: m_impl->levels) {
                    for (Impl::Node& node: level) {
                        scene.beginSystem(node.access, node.commands, context.tick);
                        try {
                            if (!node.eventHandlers.empty()) {
                                TE_PROFILER_SCOPE("SerialExecutor.EventHandlers");
                                for (const TaskGraphEventHandler& eventHandler: node.eventHandlers) {
                                    const std::span<const std::byte> batch = scene.readEventBytes(eventHandler.eventType);
                                    if (batch.empty()) {
                                        continue;
                                    }
                                    eventHandler.handler(scene, batch);
                                }
                            }
                            if (!node.inputHandlers.empty() && (!context.input.events.empty() || !m_impl->holds.empty())) {
                                TE_PROFILER_SCOPE("SerialExecutor.InputHandlers");
                                for (const InputEvent& event: context.input.events) {
                                    const InputNotification notification = m_impl->toNotification(event);
                                    for (const InputHandler& handler: node.inputHandlers) {
                                        handler(scene, notification);
                                    }
                                }
                                for (const InputNotification& hold: m_impl->holds) {
                                    for (const InputHandler& handler: node.inputHandlers) {
                                        handler(scene, hold);
                                    }
                                }
                            }

                            node.system->tick(scene, context);
                        } catch (...) {
                            scene.endSystem();
                            throw;
                        }
                        scene.endSystem();
                    }
                }
            }
        } catch (...) {
            for (Impl::Level& level: m_impl->levels) {
                for (Impl::Node& node: level) {
                    node.commands.discard();
                }
            }
            throw;
        }

        {
            TE_PROFILER_SCOPE("SerialExecutor.ApplyCommands");
            m_impl->spawned.clear();
            for (Impl::Level& level: m_impl->levels) {
                for (Impl::Node& node: level) {
                    scene.applyCommands(node.commands, m_impl->spawned);
                }
            }
        }
        {
            TE_PROFILER_SCOPE("SerialExecutor.BarrierServices");
            barrier.assignNetIds(scene, m_impl->spawned);
            scene.retireEvents();
            scene.makeEventsVisible(context.tick);
        }
    }

}
