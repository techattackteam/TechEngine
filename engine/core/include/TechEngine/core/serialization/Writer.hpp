#pragma once

#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace TechEngine {
    class Writer {
    private:
        std::vector<std::byte>& m_buffer;

    public:
        explicit Writer(std::vector<std::byte>& buffer);

        Writer(const Writer&) = delete;

        Writer& operator=(const Writer&) = delete;

        void writeHeader(std::uint16_t flags = 0);

        void write(bool value);

        void write(std::int8_t value);

        void write(std::uint8_t value);

        void write(std::int16_t value);

        void write(std::uint16_t value);

        void write(std::int32_t value);

        void write(std::uint32_t value);

        void write(std::int64_t value);

        void write(std::uint64_t value);

        void write(float value);

        void write(double value);

        void write(std::string_view value);

        void write(const char* value);

        void write(StringId value);

        void writeBytes(std::span<const std::byte> bytes);

        template<typename T>
        void writeSpan(std::span<const T> values) {
            static_assert(std::is_trivially_copyable_v<T>, "writeSpan is the bulk path; a non-trivially-copyable type must go through visit instead.");

            if (values.size() > BLOB_MAX_COUNT) {
                TE_CHECK(false, "Bulk span of {0} elements exceeds the u32 count prefix; writing an empty span", values.size());
                write(static_cast<std::uint32_t>(0));
                return;
            }

            write(static_cast<std::uint32_t>(values.size()));
            writeBytes(std::as_bytes(values));
        }

        std::size_t size() const;

    private:
        void appendRaw(const void* data, std::size_t byteCount);
    };
}
