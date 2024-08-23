#pragma once

#include "Panel.hpp"
#include "systems/SystemsRegistry.hpp"
#include "logger/ImGuiSink.hpp"

namespace TechEngine {
    class SystemsRegistry;

    class LoggerPanel : public Panel {
    private:
        SystemsRegistry& m_editorSystemsRegistry;
        SystemsRegistry& m_clientSystemsRegistry;
        SystemsRegistry& m_serverSystemsRegistry;

        bool m_showTime = true;
        bool m_showLevel = true;
        bool m_showLogger = true;

        bool m_editorEnabled = true;
        bool m_clientEnabled = true;
        bool m_serverEnabled = true;

    public:
        std::shared_ptr<ImGuiSink<std::mutex>> m_sink;

        LoggerPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& clientSystemsRegistry, SystemsRegistry& serverSystemsRegistry);

        void onInit() override;

        void onUpdate() override;

    private:
        void showLogFormatPopup();

        void showLogLevelsPopup();
    };
}
