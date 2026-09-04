#pragma once

#include <TechEngine/app/App.hpp>

#include <type_traits>
#include <utility>

namespace TechEngine {
    template<typename AppType, typename... Args>
    int runApp(Args&&... args) {
        static_assert(std::is_base_of_v<App, AppType>, "runApp<T> needs T to derive from App.");

        AppType app(std::forward<Args>(args)...);
        return app.run();
    }
}
