#include <TechEngine/base/diagnostics/StoredLogRecord.hpp>

namespace TechEngine {
    StoredLogRecord::StoredLogRecord(const LogRecord& record) : time(record.time), tick(record.tick), level(record.level), moduleTag(record.moduleTag), channel(record.channel), message(record.message), file(record.file), function(record.function), line(record.line) {
    }
}
