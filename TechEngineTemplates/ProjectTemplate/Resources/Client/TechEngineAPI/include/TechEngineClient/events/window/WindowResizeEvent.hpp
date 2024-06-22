#pragma once

#include "events/Event.hpp"

namespace TechEngine {
    class WindowResizeEvent : public TechEngine::Event {
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