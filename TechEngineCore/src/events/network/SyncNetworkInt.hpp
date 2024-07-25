#pragma once

#include <utility>

#include "events/Event.hpp"

namespace TechEngine {
    class SyncNetworkInt : public Event {
    private:
        unsigned int owner;
        std::string networkUUID;
        std::string name;
        int value;

    public:
        inline static EventType eventType = EventType("SyncNetworkInt", SYNC);

        explicit SyncNetworkInt(unsigned int owner,
                                std::string networkUUID,
                                std::string name,
                                int value) : owner(owner),
                                             networkUUID(std::move(networkUUID)),
                                             name(std::move(name)),
                                             value(value),
                                             Event(eventType) {
        }

        ~SyncNetworkInt() override = default;

        int getValue() const {
            return value;
        }

        const unsigned int& getOwner() const {
            return owner;
        }

        const std::string& getNetworkUUID() const {
            return networkUUID;
        }

        const std::string& getName() const {
            return name;
        }
    };
}
