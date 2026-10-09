#pragma once

#include <exception>
#include <format>
#include <iterator>
#include <string_view>

namespace TechEngine {
    namespace internal {
        inline constexpr std::string_view TRUNCATION_MARKER = "...[truncated]";

        struct FormatBuffer {
            char* data = nullptr;
            std::size_t capacity = 0;
            std::size_t size = 0;
            bool truncated = false;

            void push(char c) {
                if (size < capacity) {
                    data[size++] = c;
                } else {
                    truncated = true;
                }
            }

            void markTruncated() {
                if (!truncated) {
                    return;
                }
                size = size > TRUNCATION_MARKER.size() ? size - TRUNCATION_MARKER.size() : 0;
                for (const char c: TRUNCATION_MARKER) {
                    push(c);
                }
            }

            void writeFormatted(std::string_view formatString, std::format_args args);
        };

        class FormatBufferIterator {
        public:
            using difference_type = std::ptrdiff_t;
            using value_type = char;

            FormatBufferIterator() = default;

            explicit FormatBufferIterator(FormatBuffer& buffer) : m_buffer(&buffer) {
            }

            const FormatBufferIterator& operator*() const {
                return *this;
            }

            const FormatBufferIterator& operator=(char c) const {
                m_buffer->push(c);
                return *this;
            }

            FormatBufferIterator& operator++() {
                return *this;
            }

            FormatBufferIterator operator++(int) {
                return *this;
            }

        private:
            FormatBuffer* m_buffer = nullptr;
        };

        static_assert(std::output_iterator<FormatBufferIterator, char>, "vformat_to needs this to model output_iterator");

        inline void FormatBuffer::writeFormatted(std::string_view formatString, std::format_args args) {
            try {
                std::vformat_to(FormatBufferIterator{*this}, formatString, args);
            } catch (const std::exception& e) {
                size = 0;
                truncated = false;
                for (const char c: std::string_view{"<format error: "}) {
                    push(c);
                }
                for (const char c: std::string_view{e.what()}) {
                    push(c);
                }
                push('>');
            }

            markTruncated();
        }
    }
}
