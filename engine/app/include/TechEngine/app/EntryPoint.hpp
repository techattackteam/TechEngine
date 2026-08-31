#pragma once

#include <TechEngine/app/App.hpp>

#include <type_traits>

namespace TechEngine {
    template<typename AppType>
    int runApp() {
        static_assert(std::is_base_of_v<App, AppType>, "runApp<T> needs T to derive from App.");

        AppType app;
        return app.run();
    }
}
