#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace TechEngine {
    enum class ReadStatus : std::uint8_t {
        Ok,
        Truncated,
        BadMagic,
        BadVersion,
    };

    class Reader {
    private:
        std::span<const std::byte> m_buffer;
        std::size_t m_position = 0;
        ReadStatus m_status = ReadStatus::Ok;

    public:
        explicit Reader(std::span<const std::byte> buffer);

        Reader(const Reader&) = delete;

        Reader& operator=(const Reader&) = delete;

        void readHeader(BlobHeader& out);

        void read(bool& out);

        void read(std::int8_t& out);

        void read(std::uint8_t& out);

        void read(std::int16_t& out);

        void read(std::uint16_t& out);

        void read(std::int32_t& out);

        void read(std::uint32_t& out);

        void read(std::int64_t& out);

        void read(std::uint64_t& out);

        void read(float& out);

        void read(double& out);

        void read(std::string& out);

        void read(StringId& out);

        void readBytes(std::span<std::byte> out);

        template<typename T>
        void readSpan(std::vector<T>& out) {
            static_assert(std::is_trivially_copyable_v<T>, "readSpan is the bulk path; a non-trivially-copyable type must go through visit instead.");

            std::uint32_t count = 0;
            read(count);
            if (!ok()) {
                return;
            }

            if (!hasRemaining(static_cast<std::size_t>(count) * sizeof(T))) {
                fail(ReadStatus::Truncated);
                return;
            }

            out.resize(count);
            readBytes(std::as_writable_bytes(std::span<T>{out}));
        }

        bool ok() const;

        ReadStatus status() const;

        std::size_t remaining() const;

    private:
        void takeRaw(void* out, std::size_t byteCount);

        bool hasRemaining(std::size_t byteCount) const;

        void fail(ReadStatus status);
    };
}
