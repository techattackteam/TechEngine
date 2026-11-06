#include <edit/ConsoleHistory.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <utility>

namespace TechEngine {
    void ConsoleHistory::add(std::vector<StoredLogRecord> records, const std::size_t dropped) {
        if (dropped > 0) {
            addDroppedRow(dropped);
        }
        for (StoredLogRecord& record: records) {
            push(std::move(record));
        }
        rebuildVisibleRows();
    }

    void ConsoleHistory::clear() {
        m_records.clear();
        std::fill(m_levelCounts.begin(), m_levelCounts.end(), 0);
        m_visibleRows.clear();
    }

    void ConsoleHistory::setLevelShown(const Level level, const bool shown) {
        m_levelShown[static_cast<std::size_t>(level)] = shown;
        rebuildVisibleRows();
    }

    bool ConsoleHistory::isLevelShown(const Level level) const {
        return m_levelShown[static_cast<std::size_t>(level)];
    }

    void ConsoleHistory::setSearch(const std::string_view search) {
        m_search = search;
        rebuildVisibleRows();
    }

    const std::string& ConsoleHistory::getSearch() const {
        return m_search;
    }

    std::size_t ConsoleHistory::getCount(const Level level) const {
        return m_levelCounts[static_cast<std::size_t>(level)];
    }

    std::size_t ConsoleHistory::getRecordCount() const {
        return m_records.size();
    }

    std::size_t ConsoleHistory::getVisibleCount() const {
        return m_visibleRows.size();
    }

    const StoredLogRecord& ConsoleHistory::getVisibleRecord(const std::size_t row) const {
        return m_records[m_visibleRows[row]];
    }

    void ConsoleHistory::addDroppedRow(const std::size_t dropped) {
        const std::string message = std::to_string(dropped) + " records dropped";
        LogRecord droppedRecord;
        droppedRecord.time = std::chrono::system_clock::now();
        droppedRecord.level = Level::Info;
        droppedRecord.message = message;
        push(StoredLogRecord(droppedRecord));
    }

    void ConsoleHistory::push(StoredLogRecord record) {
        if (m_records.size() >= RECORD_CAPACITY) {
            const Level oldestLevel = m_records.front().level;
            m_records.pop_front();
            m_levelCounts[static_cast<std::size_t>(oldestLevel)]--;
        }
        m_levelCounts[static_cast<std::size_t>(record.level)]++;
        m_records.push_back(std::move(record));
    }

    bool ConsoleHistory::passesFilter(const StoredLogRecord& record) const {
        if (!m_levelShown[static_cast<std::size_t>(record.level)]) {
            return false;
        }
        return matchesSearch(record.message);
    }

    bool ConsoleHistory::matchesSearch(const std::string_view message) const {
        if (m_search.empty()) {
            return true;
        }
        const auto match = std::search(message.begin(), message.end(), m_search.begin(), m_search.end(), [](const char left, const char right) {
            return std::tolower(static_cast<unsigned char>(left)) == std::tolower(static_cast<unsigned char>(right));
        });
        return match != message.end();
    }

    void ConsoleHistory::rebuildVisibleRows() {
        m_visibleRows.clear();
        for (std::size_t i = 0; i < m_records.size(); i++) {
            if (passesFilter(m_records[i])) {
                m_visibleRows.push_back(i);
            }
        }
    }
}
