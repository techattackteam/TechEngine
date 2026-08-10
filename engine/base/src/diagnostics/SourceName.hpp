#pragma once

#include <cstddef>
#include <string_view>

namespace TechEngine {
    namespace detail {
        inline std::string_view baseName(std::string_view path) {
            const auto slash = path.find_last_of("/\\");
            return slash == std::string_view::npos ? path : path.substr(slash + 1);
        }

        // MSVC's function_name() is a whole signature — "int __cdecl main(void)". Walk back
        // from the first '(' over name characters to recover just the name.
        inline std::string_view shortFunctionName(std::string_view signature) {
            const auto paren = signature.find('(');
            if (paren == std::string_view::npos) {
                return signature;
            }

            const auto isNameChar = [](char c) {
                return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == ':' || c == '~' || c == '<' || c == '>';
            };

            std::size_t begin = paren;
            while (begin > 0 && isNameChar(signature[begin - 1])) {
                --begin;
            }

            return begin == paren ? signature : signature.substr(begin, paren - begin);
        }
    }
}
