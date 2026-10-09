#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Visit.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

namespace TechEngine {
    class Reader {
    private:
        std::span<const std::byte> m_buffer;
        std::size_t m_position = 0;
        std::error_code m_error;

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
                fail(ReadError::Truncated);
                return;
            }

            out.resize(count);
            readBytes(std::as_writable_bytes(std::span<T>{out}));
        }

        template<typename T>
        void field(T& value) {
            if constexpr (requires(Reader& archive) { archive.read(value); }) {
                read(value);
            } else {
                static_assert(Visitable<T, Reader>, "No read overload and no visit(archive, value) for this type; give it a visit function in its own namespace.");
                visit(*this, value);
            }
        }

        template<typename T>
        void field(std::vector<T>& values) {
            readSpan(values);
        }

        bool ok() const;

        std::error_code error() const;

        std::size_t remaining() const;

    private:
        void takeRaw(void* out, std::size_t byteCount);

        bool hasRemaining(std::size_t byteCount) const;

        void fail(ReadError reason);
    };
}
