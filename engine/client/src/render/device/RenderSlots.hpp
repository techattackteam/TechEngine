#pragma once

#include <TechEngine/base/diagnostics/Assert.hpp>

#include <render/device/RenderHandle.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace TechEngine {
    template<typename Handle, typename Payload>
    class RenderSlots {
    private:
        struct Slot {
            Payload payload{};
            std::uint32_t generation = 0;
            std::uint32_t nextFree = RENDER_NULL_INDEX;
            bool occupied = false;
        };

        std::vector<Slot> m_slots;
        std::uint32_t m_freeHead = RENDER_NULL_INDEX;
        std::uint32_t m_generationLimit = std::numeric_limits<std::uint32_t>::max();
        std::size_t m_liveCount = 0;

    public:
        explicit RenderSlots(const std::uint32_t generationLimit = std::numeric_limits<std::uint32_t>::max()) : m_generationLimit(generationLimit) {
        }

        Handle create(const Payload& payload) {
            const std::uint32_t reused = m_freeHead;
            if (reused != RENDER_NULL_INDEX) {
                Slot& slot = m_slots[reused];
                m_freeHead = slot.nextFree;
                slot.payload = payload;
                slot.occupied = true;
                slot.nextFree = RENDER_NULL_INDEX;
                m_liveCount++;
                return Handle{reused, slot.generation};
            }

            TE_CHECK(m_slots.size() < RENDER_NULL_INDEX, "Render slot limit exceeded after {0} slots", m_slots.size());
            const auto appended = static_cast<std::uint32_t>(m_slots.size());
            m_slots.push_back(Slot{payload, 0, RENDER_NULL_INDEX, true});
            m_liveCount++;
            return Handle{appended, m_slots[appended].generation};
        }

        bool destroy(const Handle handle) {
            if (!handle.valid()) {
                return false;
            }
            const auto index = handle.index;
            if (index >= m_slots.size()) {
                return false;
            }
            auto& slot = m_slots[index];
            if (!slot.occupied || slot.generation != handle.generation) {
                return false;
            }
            slot.payload = Payload{};
            slot.occupied = false;
            m_liveCount--;
            if (slot.generation == m_generationLimit) {
                return true;
            }
            slot.generation++;
            slot.nextFree = m_freeHead;
            m_freeHead = index;
            return true;
        }

        // A create() can reallocate the slots, so the pointer is dead after the next create().
        Payload* find(const Handle handle) {
            return const_cast<Payload*>(std::as_const(*this).find(handle));
        }

        const Payload* find(const Handle handle) const {
            if (!handle.valid() || handle.index >= m_slots.size()) {
                return nullptr;
            }
            const Slot& slot = m_slots[handle.index];
            if (!slot.occupied || slot.generation != handle.generation) {
                return nullptr;
            }
            return &slot.payload;
        }

        std::size_t size() const {
            return m_liveCount;
        }

        template<typename Function>
        void destroyAll(Function&& visit) {
            for (std::uint32_t index = 0; index < m_slots.size(); index++) {
                auto& slot = m_slots[index];
                if (slot.occupied) {
                    visit(slot.payload);
                    destroy(Handle{index, slot.generation});
                }
            }
        }
    };
}
