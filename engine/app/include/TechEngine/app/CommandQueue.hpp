#pragma once

#include <TechEngine/app/Command.hpp>

#include <cstddef>
#include <memory>
#include <mutex>
#include <system_error>
#include <vector>

namespace TechEngine {
    class CommandQueue {
    private:
        std::size_t m_capacity;
        std::mutex m_mutex;
        std::vector<std::unique_ptr<Command>> m_pending;
        bool m_closed = true;

    public:
        static constexpr std::size_t DEFAULT_CAPACITY = 256;

        explicit CommandQueue(std::size_t capacity = DEFAULT_CAPACITY);

        CommandQueue(const CommandQueue&) = delete;

        CommandQueue& operator=(const CommandQueue&) = delete;

        CommandQueue(CommandQueue&&) = delete;

        CommandQueue& operator=(CommandQueue&&) = delete;

        std::error_code submit(std::unique_ptr<Command> command);

        void consume(std::vector<std::unique_ptr<Command>>& batch);

        void close();

        void open();

        std::size_t capacity() const;
    };
}
