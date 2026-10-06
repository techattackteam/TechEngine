#pragma once

#include <TechEngine/core/systems/ISystem.hpp>
#include <TechEngine/core/systems/InputNotification.hpp>

#include <array>
#include <cstdint>
#include <string>

namespace TechEngine {
    class InputLogSystem final : public ISystem {
    private:
        static constexpr std::uint32_t HOLD_LOG_INTERVAL = 60;
        std::array<std::uint32_t, KEY_COUNT> m_keyHolds{};
        std::array<std::uint32_t, MOUSE_BUTTON_COUNT> m_buttonHolds{};

    public:
        void init(ScheduleRegistration& registration) override;

        void tick(Scene& scene, const SimulationContext& context) override;

        std::string_view name() const override;

    private:
        void onInput(Scene& scene, const InputNotification& input);

        void onEdge(const std::string& label, const InputNotification& input, std::uint32_t& holds);

        void onHold(const std::string& label, std::uint32_t& holds);

        void onFocus(const InputNotification& input);

        void onRecovered(const InputNotification& input);

        std::string keyLabel(Key key) const;

        std::string buttonLabel(MouseButton button) const;
    };
}
