#pragma once

#include <catch2/catch_test_macros.hpp>

#include <set>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

namespace TechEngineTests {
    template<typename ErrorEnum>
    void checkErrorCategory(const std::span<const ErrorEnum> codes, const std::error_category& category, const std::string_view name) {
        CHECK(std::string_view{category.name()} == name);

        const std::string unknown = category.message(0);
        std::set<std::string> messages;
        for (const ErrorEnum code: codes) {
            const std::error_code error = code;
            CHECK(error);
            CHECK(&error.category() == &category);
            CHECK_FALSE(std::error_code{} == code);
            for (const ErrorEnum other: codes) {
                CHECK((error == other) == (code == other));
            }

            const std::error_code foreign{error.value(), std::generic_category()};
            CHECK_FALSE(foreign == code);

            CHECK(error.message() != unknown);
            messages.insert(error.message());
        }
        CHECK(messages.size() == codes.size());
    }
}
