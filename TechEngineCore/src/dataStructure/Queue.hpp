#pragma once

#include <mutex>
#include <queue>
#include "../network/Packet.hpp"

namespace TechEngine {
    class Queue {
    private:
        std::queue<Packet *> list;
        std::mutex mutex;
    public:
        explicit Queue();

        ~Queue();

        void push(Packet *packet);

        Packet *pop();
    };
}
