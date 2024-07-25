#pragma once

#include <utility>

#include "events/Event.hpp"

namespace TechEngine {
    class RequestNetworkVariableCreation : public Event {
    private:
        unsigned int owner;
        std::string networkUUID;
        std::string name;
        int value;

    public:
        inline static EventType eventType = EventType("RequestNetworkVariableCreation", SYNC);

        RequestNetworkVariableCreation(const unsigned int& owner,
                              std::string networkUUID,
                              std::string name,
                              int value) : owner(owner),
                                           networkUUID(std::move(networkUUID)),
                                           name(std::move(name)),
                                           value(value),
                                           Event(eventType) {
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

        const int& getValue() const {
            return value;
        }
    };
}
