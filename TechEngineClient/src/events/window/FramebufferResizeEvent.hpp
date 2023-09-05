
#include <cstdint>
#include "event/events/Event.hpp"

namespace TechEngine {
    class FramebufferResizeEvent : public Event {
    private:
        uint32_t id;
        uint32_t width;
        uint32_t height;
    public:
        inline static EventType eventType = EventType("FramebufferResizeEvent", ASYNC);

        FramebufferResizeEvent(uint32_t id, uint32_t width, uint32_t height) :
                id(id), width(width), height(height), Event(eventType) {
        }

        uint32_t &getId() {
            return id;
        }

        uint32_t &getWidth() {
            return width;
        }

        uint32_t &getHeight() {
            return height;
        }
    };
}