#pragma once

#include "event/events/Event.hpp"
#include "core/Core.hpp"

namespace TechEngine {
    class /*Engine_API*/ WindowResizeEvent : public TechEngineCore::Event {
    private:
        int width;
        int height;
    public:
        inline static EventType eventType = EventType("WindowResizeEvent", SYNC);

        explicit WindowResizeEvent(int width, int height) : Event(eventType) {
            this->width = width;
            this->height = height;
        };

        ~WindowResizeEvent() override = default;

        int getWidth() {
            return width;
        }

        int getHeight() {
            return height;
        }

    };
}
