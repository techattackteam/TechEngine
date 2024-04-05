#pragma once
#include "events/Event.hpp"
#include "material/Material.hpp"

namespace TechEngine {
    class MaterialUpdateEvent : public Event {
    private:
        Material& material;

    public:
        inline static EventType eventType = EventType("MaterialUpdateEvent", SYNC);

        explicit MaterialUpdateEvent(Material& material) : material(material), Event(eventType) {
        };

        ~MaterialUpdateEvent() override = default;

        Material& getMaterial() {
            return material;
        }

        std::string getMaterialName() {
            return material.getName();
        }
    };
}
