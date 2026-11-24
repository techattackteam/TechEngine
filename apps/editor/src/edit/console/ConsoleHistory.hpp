#pragma once

#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/StoredLogRecord.hpp>

#include <array>
#include <cstddef>
#include <deque>
#include <string>
#include <string_view>
#include <vector>

namespace TechEngine {
    class ConsoleHistory {
    private:
        static constexpr std::size_t LEVEL_COUNT = static_cast<std::size_t>(Level::Critical) + 1;

        std::deque<StoredLogRecord> m_records;
        std::array<std::size_t, LEVEL_COUNT> m_levelCounts{};
        std::array<bool, LEVEL_COUNT> m_levelShown = {true, true, true, true, true, true};
        std::string m_search;
        std::deque<std::size_t> m_visibleRows;

    public:
        static constexpr std::size_t RECORD_CAPACITY = 5000;

        void add(std::vector<StoredLogRecord> records, std::size_t dropped);

        void clear();

        void setLevelShown(Level level, bool shown);

        bool isLevelShown(Level level) const;

        void setSearch(std::string_view search);

        const std::string& getSearch() const;

        std::size_t getCount(Level level) const;

        std::size_t getRecordCount() const;

        std::size_t getVisibleCount() const;

        const StoredLogRecord& getVisibleRecord(std::size_t row) const;

    private:
        void addDroppedRow(std::size_t dropped);

        void push(StoredLogRecord record);

        bool passesFilter(const StoredLogRecord& record) const;

        bool matchesSearch(std::string_view message) const;

        void rebuildVisibleRows();
    };
}
