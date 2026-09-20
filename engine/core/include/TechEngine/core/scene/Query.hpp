#pragma once

#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/core/scene/Access.hpp>
#include <TechEngine/core/scene/ComponentStorage.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace TechEngine {
    class ArchetypeStorage;

    namespace internal {
        template<typename... Types>
        inline constexpr bool UNIQUE_TYPES = true;

        template<typename First, typename... Rest>
        inline constexpr bool UNIQUE_TYPES<First, Rest...> = ((!std::is_same_v<First, Rest>) && ...) && UNIQUE_TYPES<Rest...>;
    }

    template<typename WritableComponents, typename ReadableComponents>
    class Query;

    template<ComponentValue... Written, ComponentValue... ReadOnly>
        requires((!std::same_as<Written, Hierarchy>) && ...)
    class Query<Write<Written...>, Read<ReadOnly...>> {
    private:
        using QueryType = Query<Write<Written...>, Read<ReadOnly...>>;
        using RefreshMatches = void (*)(void*, QueryType&);
        using IterationBoundary = void (*)(void*);

        struct Match {
            const std::vector<Entity>* entities = nullptr;
            std::array<IComponentStorage*, sizeof...(Written)> writableColumns{};
            std::array<const IComponentStorage*, sizeof...(ReadOnly)> readableColumns{};
        };

        class IterationGuard {
        private:
            void* m_context = nullptr;
            IterationBoundary m_end = nullptr;

        public:
            IterationGuard(void* context, const IterationBoundary begin, const IterationBoundary end) : m_context(context), m_end(end) {
                begin(context);
            }

            ~IterationGuard() {
                m_end(m_context);
            }

            IterationGuard(const IterationGuard&) = delete;

            IterationGuard& operator=(const IterationGuard&) = delete;
        };

        friend class ArchetypeStorage;

        void* m_context = nullptr;
        RefreshMatches m_refreshMatches = nullptr;
        IterationBoundary m_beginIteration = nullptr;
        IterationBoundary m_endIteration = nullptr;
        std::uint64_t m_revision = std::numeric_limits<std::uint64_t>::max();
        std::vector<Match> m_matches;

        Query(void* context, const RefreshMatches refreshMatches, const IterationBoundary beginIteration, const IterationBoundary endIteration) : m_context(context), m_refreshMatches(refreshMatches), m_beginIteration(beginIteration), m_endIteration(endIteration) {
        }

    public:
        static_assert(sizeof...(Written) + sizeof...(ReadOnly) > 0, "A query must require at least one component type");
        static_assert(internal::UNIQUE_TYPES<Written..., ReadOnly...>, "A query cannot contain the same component type more than once");

        Query(const Query&) = delete;

        Query& operator=(const Query&) = delete;

        Query(Query&& other) noexcept : m_context(other.m_context), m_refreshMatches(other.m_refreshMatches), m_beginIteration(other.m_beginIteration), m_endIteration(other.m_endIteration), m_revision(other.m_revision), m_matches(std::move(other.m_matches)) {
            other.m_context = nullptr;
        }

        Query& operator=(Query&& other) noexcept {
            if (this == &other) {
                return *this;
            }
            m_context = other.m_context;
            m_refreshMatches = other.m_refreshMatches;
            m_beginIteration = other.m_beginIteration;
            m_endIteration = other.m_endIteration;
            m_revision = other.m_revision;
            m_matches = std::move(other.m_matches);
            other.m_context = nullptr;
            return *this;
        }

        template<typename Function>
        void each(Function&& function) {
            TE_CHECK(m_context != nullptr, "Cannot iterate a moved-from query");
            m_refreshMatches(m_context, *this);
            const IterationGuard guard(m_context, m_beginIteration, m_endIteration);

            for (const Match& match: m_matches) {
                auto writable = writableSpans(match, std::index_sequence_for<Written...>{});
                auto readable = readableSpans(match, std::index_sequence_for<ReadOnly...>{});

                for (std::size_t row = 0; row < match.entities->size(); row++) {
                    const Entity entity = (*match.entities)[row];
                    invoke(function, entity, row, writable, readable, std::index_sequence_for<Written...>{}, std::index_sequence_for<ReadOnly...>{});
                }
            }
        }

    private:
        template<typename Contains>
        bool matches(Contains&& contains) const {
            return (contains(std::type_identity<Written>{}) && ...) && (contains(std::type_identity<ReadOnly>{}) && ...);
        }

        template<typename GetColumn>
        void addMatch(const std::vector<Entity>* entities, GetColumn&& getColumn) {
            m_matches.push_back({entities, {getColumn(std::type_identity<Written>{})...}, {getColumn(std::type_identity<ReadOnly>{})...}});
        }

        template<std::size_t... Indices>
        static auto writableSpans(const Match& match, std::index_sequence<Indices...>) {
            using ComponentTypes = std::tuple<Written...>;
            return std::tuple{static_cast<ComponentStorage<std::tuple_element_t<Indices, ComponentTypes>>&>(*match.writableColumns[Indices]).values()...};
        }

        template<std::size_t... Indices>
        static auto readableSpans(const Match& match, std::index_sequence<Indices...>) {
            using ComponentTypes = std::tuple<ReadOnly...>;
            return std::tuple{static_cast<const ComponentStorage<std::tuple_element_t<Indices, ComponentTypes>>&>(*match.readableColumns[Indices]).values()...};
        }

        template<typename Function, typename WritableSpans, typename ReadableSpans, std::size_t... WritableIndices, std::size_t... ReadableIndices>
        static void invoke(Function& function, const Entity entity, const std::size_t row, WritableSpans& writable, ReadableSpans& readable, std::index_sequence<WritableIndices...>, std::index_sequence<ReadableIndices...>) {
            std::invoke(function, entity, std::get<WritableIndices>(writable)[row]..., std::get<ReadableIndices>(readable)[row]...);
        }
    };
}
