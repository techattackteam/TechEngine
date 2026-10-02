#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/systems/ScheduleRegistration.hpp>

#include <demo/InputLogSystem.hpp>

#include <cstddef>

namespace TechEngine {
    void InputLogSystem::init(ScheduleRegistration& registration) {
        registration.onInput([this](Scene& scene, const InputNotification& input) {
            onInput(scene, input);
        });
    }

    void InputLogSystem::tick(Scene&, const SimulationContext&) {
    }

    std::string_view InputLogSystem::name() const {
        return "InputLogSystem";
    }

    void InputLogSystem::onInput(Scene&, const InputNotification& input) {
        switch (input.kind) {
            case InputNotificationKind::Key:
                onEdge(keyLabel(input.key), input, m_keyHolds[static_cast<std::size_t>(input.key)]);
                break;
            case InputNotificationKind::Button:
                onEdge(buttonLabel(input.button), input, m_buttonHolds[static_cast<std::size_t>(input.button)]);
                break;
            case InputNotificationKind::Motion:
                break;
            case InputNotificationKind::Focus:
                onFocus(input);
                break;
            case InputNotificationKind::KeyHold:
                onHold(keyLabel(input.key), m_keyHolds[static_cast<std::size_t>(input.key)]);
                break;
            case InputNotificationKind::ButtonHold:
                onHold(buttonLabel(input.button), m_buttonHolds[static_cast<std::size_t>(input.button)]);
                break;
        }
    }

    void InputLogSystem::onEdge([[maybe_unused]] const std::string& label, const InputNotification& input, std::uint32_t& holds) {
        if (input.pressed) {
            TE_LOGGER_INFO("InputLogSystem: {0} PRESS (#{1}), a single edge; holds follow once per Tick while it stays down", label, input.sequence);
        } else if (holds == 0) {
            TE_LOGGER_INFO("InputLogSystem: {0} RELEASE (#{1}) in the same Tick as its press: both edges arrived, no hold", label, input.sequence);
        } else {
            TE_LOGGER_INFO("InputLogSystem: {0} RELEASE (#{1}) after {2} holds, one for each Tick that ended with it down", label, input.sequence, holds);
        }
        holds = 0;
    }

    void InputLogSystem::onHold([[maybe_unused]] const std::string& label, std::uint32_t& holds) {
        holds++;
        if (holds == 1) {
            TE_LOGGER_INFO("InputLogSystem: {0} HOLD 1, the first Tick that ended with it down", label);
        } else if (holds % HOLD_LOG_INTERVAL == 0) {
            TE_LOGGER_INFO("InputLogSystem: {0} HOLD {1}, still down with no new event", label, holds);
        }
    }

    void InputLogSystem::onFocus([[maybe_unused]] const InputNotification& input) {
        TE_LOGGER_INFO("InputLogSystem: FOCUS {0} (#{1}), held controls reset; GLFW's synthetic releases will not arrive", input.pressed ? "GAINED" : "LOST", input.sequence);
        for (std::size_t i = 0; i < KEY_COUNT; i++) {
            if (m_keyHolds[i] > 0) {
                TE_LOGGER_INFO("InputLogSystem: {0} cleared by the focus change after {1} holds, without a RELEASE", keyLabel(static_cast<Key>(i)), m_keyHolds[i]);
            }
        }
        for (std::size_t i = 0; i < MOUSE_BUTTON_COUNT; i++) {
            if (m_buttonHolds[i] > 0) {
                TE_LOGGER_INFO("InputLogSystem: {0} cleared by the focus change after {1} holds, without a RELEASE", buttonLabel(static_cast<MouseButton>(i)), m_buttonHolds[i]);
            }
        }
        m_keyHolds.fill(0);
        m_buttonHolds.fill(0);
    }

    std::string InputLogSystem::keyLabel(const Key key) const {
        if (key >= Key::A && key <= Key::Z) {
            return std::string(1, static_cast<char>('A' + (static_cast<int>(key) - static_cast<int>(Key::A))));
        }
        return "key " + std::to_string(static_cast<int>(key));
    }

    std::string InputLogSystem::buttonLabel(const MouseButton button) const {
        switch (button) {
            case MouseButton::Left:
                return "mouse Left";
            case MouseButton::Right:
                return "mouse Right";
            case MouseButton::Middle:
                return "mouse Middle";
            default:
                return "mouse button " + std::to_string(static_cast<int>(button));
        }
    }
}
