#pragma once

#include <utility>

#include "events/Event.hpp"
#include "scriptingAPI/network/NetworkObject.hpp"

namespace TechEngine {
    class NetworkIntValueChanged : public Event {
    private:
        unsigned int owner;
        std::string networkUUID;
        std::string name;
        int value;

    public:
        inline static EventType eventType = EventType("NetworkIntValueChanged", SYNC);


        NetworkIntValueChanged(unsigned int owner,
                               std::string string,
                               std::string name,
                               int value) : owner(owner),
                                            networkUUID(std::move(string)),
                                            name(std::move(name)),
                                            value(value),
                                            Event(eventType) {
        }


        ~NetworkIntValueChanged() override = default;

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
