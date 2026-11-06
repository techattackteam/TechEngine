#pragma once

#include <TechEngine/base/diagnostics/StoredLogRecord.hpp>

#include <ui/panels/Panel.hpp>

#include <array>
#include <cstddef>
#include <optional>

namespace TechEngine {
    class ConsoleHistory;

    class ConsolePanel final : public Panel {
    private:
        static constexpr std::size_t SEARCH_CAPACITY = 256;

        ConsoleHistory* m_history = nullptr;
        // A copy, because the history drops its oldest record and the selection must outlive it.
        std::optional<StoredLogRecord> m_selected;
        std::array<char, SEARCH_CAPACITY> m_search{};
        std::size_t m_lastVisibleCount = 0;
        bool m_atBottom = true;
        bool m_showTime = true;
        bool m_showTick = true;

    public:
        explicit ConsolePanel(ConsoleHistory& history);

    protected:
        void onDraw(const EditorSnapshot& snapshot) override;

    private:
        void drawToolbar();

        void drawShowMenu();

        void drawRows();

        void drawRow(int row, const StoredLogRecord& record);

        bool isSelected(const StoredLogRecord& record) const;

        void drawSelectedSource();
    };
}
