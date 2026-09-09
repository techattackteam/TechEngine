#pragma once

#include <exception>
#include <future>
#include <stop_token>
#include <utility>

namespace TechEngine {
    template<typename Thread, typename RegistrationFactory, typename Entry, typename Completion>
    static Thread createRegisteredThread(RegistrationFactory registerThread, Entry entry, Completion complete) {
        std::promise<void> registered;
        std::future<void> ready = registered.get_future();
        Thread thread{[registerThread = std::move(registerThread), entry = std::move(entry), complete = std::move(complete), registered = std::move(registered)](const std::stop_token stopToken = {}) mutable {
            std::exception_ptr failure;
            try {
                const auto registration = registerThread();
                registered.set_value();
                try {
                    entry(stopToken);
                } catch (...) {
                    failure = std::current_exception();
                }
            } catch (...) {
                registered.set_exception(std::current_exception());
                return;
            }
            complete(failure);
        }};
        try {
            ready.get();
        } catch (...) {
            thread.join();
            throw;
        }
        return thread;
    }
}
