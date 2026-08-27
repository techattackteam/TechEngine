#include <TechEngine/core/serialization/Reader.hpp>

#include <cstring>

namespace TechEngine {
    Reader::Reader(const std::span<const std::byte> buffer) : m_buffer{buffer} {
    }

    void Reader::readHeader(BlobHeader& out) {
        std::uint32_t magic = 0;
        read(magic);
        if (!ok()) {
            return;
        }
        if (magic != BLOB_MAGIC) {
            fail(ReadStatus::BadMagic);
            return;
        }

        std::uint16_t formatVersion = 0;
        read(formatVersion);
        if (!ok()) {
            return;
        }
        if (formatVersion != BLOB_FORMAT_VERSION) {
            fail(ReadStatus::BadVersion);
            return;
        }

        std::uint16_t flags = 0;
        read(flags);
        if (!ok()) {
            return;
        }

        out.magic = magic;
        out.formatVersion = formatVersion;
        out.flags = flags;
    }

    void Reader::read(bool& out) {
        std::uint8_t encoded = 0;
        read(encoded);
        if (!ok()) {
            return;
        }

        out = encoded != 0;
    }

    void Reader::read(std::int8_t& out) {
        takeRaw(&out, sizeof(out));
    }

    void Reader::read(std::uint8_t& out) {
        takeRaw(&out, sizeof(out));
    }

    void Reader::read(std::int16_t& out) {
        takeRaw(&out, sizeof(out));
    }

    void Reader::read(std::uint16_t& out) {
        takeRaw(&out, sizeof(out));
    }

    void Reader::read(std::int32_t& out) {
        takeRaw(&out, sizeof(out));
    }

    void Reader::read(std::uint32_t& out) {
        takeRaw(&out, sizeof(out));
    }

    void Reader::read(std::int64_t& out) {
        takeRaw(&out, sizeof(out));
    }

    void Reader::read(std::uint64_t& out) {
        takeRaw(&out, sizeof(out));
    }

    void Reader::read(float& out) {
        takeRaw(&out, sizeof(out));
    }

    void Reader::read(double& out) {
        takeRaw(&out, sizeof(out));
    }

    void Reader::read(std::string& out) {
        std::uint32_t length = 0;
        read(length);
        if (!ok()) {
            return;
        }

        if (!hasRemaining(length)) {
            fail(ReadStatus::Truncated);
            return;
        }

        out.resize(length);
        takeRaw(out.data(), length);
    }

    void Reader::read(StringId& out) {
        std::uint64_t raw = 0;
        read(raw);
        if (!ok()) {
            return;
        }

        out = StringId::fromValue(raw);
    }

    void Reader::readBytes(const std::span<std::byte> out) {
        takeRaw(out.data(), out.size());
    }

    bool Reader::ok() const {
        return m_status == ReadStatus::Ok;
    }

    ReadStatus Reader::status() const {
        return m_status;
    }

    std::size_t Reader::remaining() const {
        return m_buffer.size() - m_position;
    }

    void Reader::takeRaw(void* out, const std::size_t byteCount) {
        if (!ok()) {
            return;
        }
        if (remaining() < byteCount) {
            fail(ReadStatus::Truncated);
            return;
        }
        std::memcpy(out, m_buffer.data() + m_position, byteCount);
        m_position += byteCount;
    }

    bool Reader::hasRemaining(const std::size_t byteCount) const {
        if (ok()) {
            return remaining() >= byteCount;
        }
        return false;
    }

    void Reader::fail(const ReadStatus status) {
        if (ok()) {
            m_status = status;
        }
    }
}
