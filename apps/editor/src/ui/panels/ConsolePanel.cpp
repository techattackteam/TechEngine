#include <TechEngine/base/diagnostics/Log.hpp>

#include <edit/ConsoleHistory.hpp>
#include <ui/panels/ConsolePanel.hpp>

#include <imgui.h>

#include <array>
#include <cfloat>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <format>
#include <string>
#include <string_view>

namespace TechEngine {
    static constexpr std::array<Level, 6> CONSOLE_LEVELS = {Level::Trace, Level::Debug, Level::Info, Level::Warn, Level::Error, Level::Critical};
    static constexpr ImVec4 DEBUG_COLOR = ImVec4(0.45f, 0.65f, 0.95f, 1.00f);
    static constexpr ImVec4 WARN_COLOR = ImVec4(0.95f, 0.80f, 0.30f, 1.00f);
    static constexpr ImVec4 ERROR_COLOR = ImVec4(0.95f, 0.40f, 0.35f, 1.00f);
    static constexpr ImVec4 CRITICAL_COLOR = ImVec4(1.00f, 0.25f, 0.55f, 1.00f);
    static constexpr ImVec4 ROWS_BACKGROUND = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    static constexpr ImVec2 ROWS_PADDING = ImVec2(6.0f, 4.0f);

    static const char* levelLabel(const Level level) {
        switch (level) {
            case Level::Trace:
                return "Trace";
            case Level::Debug:
                return "Debug";
            case Level::Info:
                return "Info";
            case Level::Warn:
                return "Warn";
            case Level::Error:
                return "Error";
            case Level::Critical:
                return "Critical";
            case Level::Off:
                return "Off";
        }
        return "Info";
    }

    static bool levelColor(const Level level, ImVec4& out) {
        switch (level) {
            case Level::Debug:
                out = DEBUG_COLOR;
                return true;
            case Level::Warn:
                out = WARN_COLOR;
                return true;
            case Level::Error:
                out = ERROR_COLOR;
                return true;
            case Level::Critical:
                out = CRITICAL_COLOR;
                return true;
            case Level::Trace:
            case Level::Info:
            case Level::Off:
                return false;
        }
        return false;
    }

    // std::localtime shares one static tm across threads; the CRT's reentrant spellings do not.
    static std::tm localTime(const std::time_t seconds) {
        std::tm out{};
#if defined(_WIN32)
        localtime_s(&out, &seconds);
#else
        localtime_r(&seconds, &out);
#endif
        return out;
    }

    ConsolePanel::ConsolePanel(ConsoleHistory& history) : Panel("Console"), m_history(&history) {
    }

    void ConsolePanel::onDraw(const EditorSnapshot&) {
        drawToolbar();
        drawRows();
        drawSelectedSource();
    }

    void ConsolePanel::drawToolbar() {
        if (ImGui::Button("Clear")) {
            m_history->clear();
            m_selected.reset();
        }
        ImGui::SameLine();
        drawShowMenu();
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputTextWithHint("##search", "Search", m_search.data(), m_search.size())) {
            m_history->setSearch(m_search.data());
        }
    }

    void ConsolePanel::drawShowMenu() {
        std::size_t shownLevels = 0;
        for (const Level level: CONSOLE_LEVELS) {
            if (m_history->isLevelShown(level)) {
                shownLevels++;
            }
        }
        const std::string preview = shownLevels == CONSOLE_LEVELS.size() ? std::string("All levels") : std::format("{0} of {1} levels", shownLevels, CONSOLE_LEVELS.size());
        const ImGuiStyle& style = ImGui::GetStyle();
        ImGui::SetNextItemWidth(ImGui::CalcTextSize("6 of 6 levels").x + style.FramePadding.x * 2.0f + ImGui::GetFrameHeight());
        if (!ImGui::BeginCombo("##show", preview.c_str())) {
            return;
        }
        ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
        ImGui::SeparatorText("Levels");
        for (const Level level: CONSOLE_LEVELS) {
            bool shown = m_history->isLevelShown(level);
            const std::string count = std::to_string(m_history->getCount(level));
            if (ImGui::MenuItem(levelLabel(level), count.c_str(), &shown)) {
                m_history->setLevelShown(level, shown);
            }
        }
        ImGui::SeparatorText("Columns");
        ImGui::MenuItem("Time", nullptr, &m_showTime);
        ImGui::MenuItem("Tick", nullptr, &m_showTick);
        ImGui::PopItemFlag();
        ImGui::EndCombo();
    }

    void ConsolePanel::drawRows() {
        const float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        const float rowHeight = ImGui::GetTextLineHeightWithSpacing();
        const std::size_t visibleCount = m_history->getVisibleCount();
        // SetScrollHereY lands only on the next frame, and an idle editor may not build one, so a new
        // row would stay hidden. Setting the scroll and the content size it clamps against before the
        // child begins scrolls in this frame.
        if (m_atBottom && visibleCount != m_lastVisibleCount) {
            ImGui::SetNextWindowContentSize(ImVec2(0.0f, static_cast<float>(visibleCount) * rowHeight));
            ImGui::SetNextWindowScroll(ImVec2(-1.0f, FLT_MAX));
        }
        m_lastVisibleCount = visibleCount;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ROWS_BACKGROUND);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ROWS_PADDING);
        const bool rowsVisible = ImGui::BeginChild("ConsoleRows", ImVec2(0.0f, -footerHeight), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        if (rowsVisible) {
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(visibleCount), rowHeight);
            while (clipper.Step()) {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                    drawRow(row, m_history->getVisibleRecord(static_cast<std::size_t>(row)));
                }
            }
            m_atBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
        }
        ImGui::EndChild();
    }

    void ConsolePanel::drawRow(const int row, const StoredLogRecord& record) {
        ImGui::PushID(row);
        const float rowStart = ImGui::GetCursorPosX();
        if (ImGui::Selectable("##row", isSelected(record))) {
            m_selected = record;
        }
        ImGui::SameLine(rowStart);

        std::string prefix;
        if (m_showTime) {
            const auto sinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(record.time.time_since_epoch());
            const std::tm local = localTime(std::chrono::system_clock::to_time_t(record.time));
            prefix = std::format("{0:02}:{1:02}:{2:02}.{3:03} ", local.tm_hour, local.tm_min, local.tm_sec, static_cast<int>(sinceEpoch.count() % 1000));
        }
        if (m_showTick) {
            prefix += std::format("[t {0}] ", record.tick);
        }
        if (!prefix.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextUnformatted(prefix.data(), prefix.data() + prefix.size());
            ImGui::PopStyleColor();
            ImGui::SameLine(0.0f, 0.0f);
        }

        // The clipper assumes every row is one line high, so a multi-line message shows its first line.
        const std::string_view message = std::string_view(record.message).substr(0, record.message.find('\n'));
        const std::string text = std::format("[{0:<4}] {1}", levelLabel(record.level), message);

        ImVec4 color;
        const bool colored = levelColor(record.level, color);
        if (colored) {
            ImGui::PushStyleColor(ImGuiCol_Text, color);
        }
        ImGui::TextUnformatted(text.data(), text.data() + text.size());
        if (colored) {
            ImGui::PopStyleColor();
        }
        ImGui::PopID();
    }

    bool ConsolePanel::isSelected(const StoredLogRecord& record) const {
        if (!m_selected.has_value()) {
            return false;
        }
        const StoredLogRecord& selected = *m_selected;
        return selected.time == record.time && selected.tick == record.tick && selected.line == record.line && selected.file == record.file && selected.message == record.message;
    }

    void ConsolePanel::drawSelectedSource() {
        if (!m_selected.has_value()) {
            ImGui::TextDisabled("Select a row to see where it was logged.");
            return;
        }
        const StoredLogRecord& record = *m_selected;
        if (record.file.empty()) {
            ImGui::TextDisabled("This row has no source.");
            return;
        }
        const std::string source = std::format("{0}:{1}:{2}()  {3}/{4}", record.file, record.line, record.function, logModuleName(record.moduleTag), logChannelName(record.channel));
        ImGui::TextUnformatted(source.data(), source.data() + source.size());
    }
}
