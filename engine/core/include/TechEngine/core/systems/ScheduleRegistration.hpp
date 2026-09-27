#pragma once

#include <TechEngine/core/events/EventTypeId.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/systems/ISystem.hpp>
#include <TechEngine/core/systems/ScheduleAccess.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <typeindex>
#include <utility>

namespace TechEngine {
    class Scene;

    using EventHandler = std::function<void(Scene&, std::span<const std::byte>)>;

    struct EventHandlerDeclaration {
        EventTypeId (*eventType)() = nullptr;
        EventHandler handler;
    };

    enum class Slot : std::uint8_t {
        Regular,
        Terminal,
    };

    enum class Order : std::uint8_t {
        Before,
        After,
    };

    class Schedule;

    class ScheduleRegistration {
    private:
        Schedule* m_schedule = nullptr;
        std::size_t m_entryIndex = 0;

        ScheduleRegistration(Schedule& schedule, std::size_t entryIndex);

        friend class Schedule;

    public:
        template<typename... Written, typename... ReadOnly>
            requires((!std::same_as<Written, Hierarchy>) && ...)
        ScheduleRegistration& access(DeclareAccess<Write<Written...>, Read<ReadOnly...>>) {
            const std::array<ComponentTypeId, sizeof...(Written)> written{componentTypeId<Written>()...};
            const std::array<ComponentTypeId, sizeof...(ReadOnly)> readOnly{componentTypeId<ReadOnly>()...};
            addAccess(written, readOnly);
            return *this;
        }

        ScheduleRegistration& setPriority(int value);

        ScheduleRegistration& setSlot(Slot value);

        template<std::derived_from<ISystem> T>
        ScheduleRegistration& before() {
            addOrder(typeid(T), Order::Before);
            return *this;
        }

        template<std::derived_from<ISystem> T>
        ScheduleRegistration& after() {
            addOrder(typeid(T), Order::After);
            return *this;
        }

        template<typename Event>
        ScheduleRegistration& on(std::function<void(Scene&, std::span<const Event>)> handler) {
            addEventHandler({&eventTypeId<Event>, [typed = std::move(handler)](Scene& scene, const std::span<const std::byte> bytes) {
                                 typed(scene, std::span<const Event>(reinterpret_cast<const Event*>(bytes.data()), bytes.size() / sizeof(Event)));
                             }});
            return *this;
        }

    private:
        void addAccess(std::span<const ComponentTypeId> written, std::span<const ComponentTypeId> readOnly);

        void addOrder(std::type_index systemType, Order order);

        void addEventHandler(EventHandlerDeclaration declaration);
    };
}
