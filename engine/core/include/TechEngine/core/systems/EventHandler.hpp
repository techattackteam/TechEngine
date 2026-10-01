#pragma once

#include <TechEngine/core/events/EventTypeId.hpp>

#include <cstddef>
#include <functional>
#include <span>

namespace TechEngine {
    class Scene;

    using EventCallback = std::function<void(Scene&, std::span<const std::byte>)>;

    struct EventHandler {
        EventTypeId (*eventType)() = nullptr;
        EventCallback handler;
    };
}
