#include "Queue.hpp"

namespace TechEngine {

    Queue::Queue() : list() {

    }

    Queue::~Queue() {

    }

    void Queue::push(Packet *packet) {
        mutex.lock();
        list.push(packet);
        mutex.unlock();
    }

    Packet *Queue::pop() {
        mutex.lock();
        Packet *packet;
        if (!list.empty()) {
            packet = list.front();
            list.pop();
        } else {
            packet = nullptr;
        }
        mutex.unlock();
        return packet;
    }

}