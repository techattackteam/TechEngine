#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

namespace TechEngine {
    Writer::Writer(std::vector<std::byte>& buffer) : m_buffer{buffer} {
    }

    void Writer::writeHeader(const std::uint16_t flags) {
        write(BLOB_MAGIC);
        write(BLOB_FORMAT_VERSION);
        write(flags);
    }

    void Writer::write(const bool value) {
        const std::uint8_t encoded = value ? 1 : 0;
        appendRaw(&encoded, sizeof(encoded));
    }

    void Writer::write(const std::int8_t value) {
        appendRaw(&value, sizeof(value));
    }

    void Writer::write(const std::uint8_t value) {
        appendRaw(&value, sizeof(value));
    }

    void Writer::write(const std::int16_t value) {
        appendRaw(&value, sizeof(value));
    }

    void Writer::write(const std::uint16_t value) {
        appendRaw(&value, sizeof(value));
    }

    void Writer::write(const std::int32_t value) {
        appendRaw(&value, sizeof(value));
    }

    void Writer::write(const std::uint32_t value) {
        appendRaw(&value, sizeof(value));
    }

    void Writer::write(const std::int64_t value) {
        appendRaw(&value, sizeof(value));
    }

    void Writer::write(const std::uint64_t value) {
        appendRaw(&value, sizeof(value));
    }

    void Writer::write(const float value) {
        appendRaw(&value, sizeof(value));
    }

    void Writer::write(const double value) {
        appendRaw(&value, sizeof(value));
    }

    void Writer::write(const std::string_view value) {
        if (!TE_ENSURE(value.size() <= BLOB_MAX_COUNT, "String of {0} bytes exceeds the u32 length prefix; writing an empty string", value.size())) {
            write(static_cast<std::uint32_t>(0));
            return;
        }

        write(static_cast<std::uint32_t>(value.size()));
        appendRaw(value.data(), value.size());
    }

    void Writer::write(const char* value) {
        write(std::string_view{value});
    }

    void Writer::write(const StringId value) {
        write(value.value());
    }

    void Writer::writeBytes(const std::span<const std::byte> bytes) {
        appendRaw(bytes.data(), bytes.size());
    }

    std::size_t Writer::size() const {
        return m_buffer.size();
    }

    void Writer::appendRaw(const void* data, const std::size_t byteCount) {
        m_buffer.insert(m_buffer.end(), static_cast<const std::byte*>(data), static_cast<const std::byte*>(data) + byteCount);
    }
}
