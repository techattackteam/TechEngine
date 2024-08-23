#include "LoggerPanel.hpp"

#include "core/Logger.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "logger/ImGuiSink.hpp"
#include "spdlog/spdlog.h"

namespace TechEngine {
    LoggerPanel::LoggerPanel(SystemsRegistry& editorSystemsRegistry,
                             SystemsRegistry& clientSystemsRegistry,
                             SystemsRegistry& serverSystemsRegistry) : m_editorSystemsRegistry(editorSystemsRegistry),
                                                                       m_clientSystemsRegistry(clientSystemsRegistry),
                                                                       m_serverSystemsRegistry(serverSystemsRegistry) {
    }

    void LoggerPanel::onInit() {
        m_sink = std::make_shared<ImGuiSink<std::mutex>>();
        m_editorSystemsRegistry.getSystem<Logger>().addLogSink(m_sink);
        m_clientSystemsRegistry.getSystem<Logger>().addLogSink(m_sink);
        m_serverSystemsRegistry.getSystem<Logger>().addLogSink(m_sink);
    }


    void LoggerPanel::onUpdate() {
        // Make all buttons transparent in the toolbar
        auto button_color = ImGui::GetStyleColorVec4(ImGuiCol_Button);
        button_color.w = 0.0F;
        ImGui::PushStyleColor(ImGuiCol_Button, button_color);

        ImGui::Checkbox("Engine", &m_editorEnabled);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enable/Disable engine logging");
        }
        ImGui::SameLine();
        ImGui::Checkbox("Client", &m_clientEnabled);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enable/Disable editor logging");
        }
        ImGui::SameLine();
        ImGui::Checkbox("Server", &m_serverEnabled);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enable/Disable editor logging");
        }

        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        if (ImGui::Button("Format")) {
            ImGui::OpenPopup("LogFormatPopup");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Chose what information to show");
        }
        if (ImGui::BeginPopup("LogFormatPopup")) {
            showLogFormatPopup();
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            m_sink->clearMessages();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Discard all messages");
        }

        // Restore the button color
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1F, 0.1F, 0.1F, 1.0F));
        ImGui::BeginChild("LogMessages", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::Dummy(ImVec2(0, 1.0f)); // Add padding from the child window's border
        for (const auto& message: m_sink->getLogMessages()) {
            spdlog::level::level_enum level = message.level;
            LogOutput logger_name = message.output;
            std::string msg = message.log;
            if (logger_name == LogOutput::Editor && !m_editorEnabled ||
                (logger_name == LogOutput::Client || logger_name == LogOutput::EngineClient) && !m_clientEnabled ||
                (logger_name == LogOutput::Server || logger_name == LogOutput::EngineServer) && !m_serverEnabled) {
                continue;
            }
            ImVec4 color;
            if (level == spdlog::level::trace) {
                color = ImVec4(0.5F, 0.5F, 0.5F, 1.0F);
            } else if (level == spdlog::level::debug) {
                color = ImVec4(0.0F, 1.0F, 0.0F, 1.0F);
            } else if (level == spdlog::level::info) {
                color = ImVec4(1.0F, 1.0F, 1.0F, 1.0F);
            } else if (level == spdlog::level::warn) {
                color = ImVec4(1.0F, 1.0F, 0.0F, 1.0F);
            } else if (level == spdlog::level::err) {
                color = ImVec4(1.0F, 0.0F, 0.0F, 1.0F);
            } else if (level == spdlog::level::critical) {
                color = ImVec4(1.0F, 0.0F, 0.0F, 1.0F);
            }
            std::string log;
            if (m_showTime) {
                log += "[" + message.time + "] ";
            }
            if (m_showLogger) {
                log += "[" + message.logger + "] ";
            }
            if (m_showLevel) {
                log += "[" + message.levelStr + "] ";
            }
            log += message.log;
            ImGui::TextColored(color, log.c_str());
        }
        ImGui::Dummy(ImVec2(0, 4.0f)); // Add padding from the child window's border
        // Remove the padding style modification
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();
        ImGui::PopStyleColor(1);
    }

    void LoggerPanel::showLogFormatPopup() {
        ImGui::MenuItem("Logging Format", nullptr, false, false);
        ImGui::Checkbox("Time", &m_showTime);
        ImGui::SameLine();
        ImGui::Checkbox("Logger", &m_showLogger);
        ImGui::SameLine();
        ImGui::Checkbox("Level", &m_showLevel);
    }

    void LoggerPanel::showLogLevelsPopup() {
    }
}
