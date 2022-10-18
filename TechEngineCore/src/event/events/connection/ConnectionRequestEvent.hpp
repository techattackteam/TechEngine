#pragma once

#include <boost/asio/ip/udp.hpp>
#include "event/EventDispatcher.hpp"
#include "core/Core.hpp"

using boost::asio::ip::udp;
namespace TechEngineCore {
    class Engine_API ConnectionRequestEvent : public Event {
    private:
        udp::endpoint endpoint;
    public:
        static inline EventType eventType = EventType("ConnectionRequestEvent", ASYNC);

        ConnectionRequestEvent(udp::endpoint endpoint) : endpoint(endpoint), Event(eventType) {
        };

        ~ConnectionRequestEvent() override = default;


        udp::endpoint getEndpoint() {
            return endpoint;
        }
    };
}
