#pragma once

#include <TechEngine/client/render/ImGuiHook.hpp>

#include <glad/gl.h>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace TechEngineTests {
    struct ImGuiHookCall {
        std::string name;
        std::thread::id thread;
        bool contextCurrent = false;
    };

    class RecordingImGuiHook final : public TechEngine::ImGuiHook {
    private:
        mutable std::mutex m_mutex;
        std::condition_variable m_changed;
        std::vector<ImGuiHookCall> m_calls;
        bool m_throwOnDraw = false;

    public:
        explicit RecordingImGuiHook(const bool throwOnDraw = false) : m_throwOnDraw(throwOnDraw) {
        }

        void start() override {
            record(ImGuiHookCall{.name = "start"});
        }

        void draw() override {
            record(ImGuiHookCall{.name = "draw"});
            if (m_throwOnDraw) {
                throw std::runtime_error{"ImGui hook draw failed"};
            }
        }

        void stop() override {
            record(ImGuiHookCall{.name = "stop"});
        }

        bool waitForDraws(const std::size_t count) const {
            std::unique_lock lock{m_mutex};
            return const_cast<std::condition_variable&>(m_changed).wait_for(lock, std::chrono::seconds{5}, [&] {
                return drawCount() >= count;
            });
        }

        std::vector<ImGuiHookCall> calls() const {
            const std::lock_guard lock{m_mutex};
            return m_calls;
        }

    private:
        void record(ImGuiHookCall call) {
            call.thread = std::this_thread::get_id();
            call.contextCurrent = glGetString(GL_VERSION) != nullptr;
            {
                const std::lock_guard lock{m_mutex};
                m_calls.push_back(std::move(call));
            }
            m_changed.notify_all();
        }

        std::size_t drawCount() const {
            std::size_t count = 0;
            for (const ImGuiHookCall& call: m_calls) {
                if (call.name == "draw") {
                    count++;
                }
            }
            return count;
        }
    };
}
