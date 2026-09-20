#pragma once

#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/core/scene/Access.hpp>
#include <TechEngine/core/scene/ComponentStorage.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
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
    class Hierarchy;
    class ArchetypeStorage;
    class Scene;

    namespace internal {
        template<typename... Types>
        inline constexpr bool UNIQUE_TYPES = true;

        template<typename First, typename... Rest>
        inline constexpr bool UNIQUE_TYPES<First, Rest...> = ((!std::is_same_v<First, Rest>) && ...) && UNIQUE_TYPES<Rest...>;

        class QuerySource {
        protected:
            ~QuerySource() = default;

        public:
            virtual std::uint64_t queryRevision() const = 0;

            virtual std::size_t queryArchetypeCount() const = 0;

            virtual bool queryArchetypeContains(std::size_t archetypeIndex, ComponentTypeId type) const = 0;

            virtual const std::vector<Entity>* queryEntities(std::size_t archetypeIndex) const = 0;

            virtual IComponentStorage* queryColumn(std::size_t archetypeIndex, ComponentTypeId type) = 0;

            virtual void beginQueryIteration() = 0;

            virtual void endQueryIteration() = 0;
        };
    }

    template<typename WritableComponents, typename ReadableComponents>
    class Query;

    template<ComponentValue... Written, ComponentValue... ReadOnly>
        requires((!std::same_as<Written, Hierarchy>) && ...)
    class Query<Write<Written...>, Read<ReadOnly...>> {
    private:
        struct Match {
            const std::vector<Entity>* entities = nullptr;
            std::array<IComponentStorage*, sizeof...(Written)> writableColumns{};
            std::array<const IComponentStorage*, sizeof...(ReadOnly)> readableColumns{};
        };

        class IterationGuard {
        private:
            internal::QuerySource* m_source = nullptr;

        public:
            explicit IterationGuard(internal::QuerySource& source) : m_source(&source) {
                m_source->beginQueryIteration();
            }

            ~IterationGuard() {
                m_source->endQueryIteration();
            }

            IterationGuard(const IterationGuard&) = delete;

            IterationGuard& operator=(const IterationGuard&) = delete;
        };

        friend class ArchetypeStorage;
        friend class Scene;

        internal::QuerySource* m_source = nullptr;
        std::uint64_t m_revision = std::numeric_limits<std::uint64_t>::max();
        std::vector<Match> m_matches;

        explicit Query(internal::QuerySource& source) : m_source(&source) {
        }

    public:
        static_assert(sizeof...(Written) + sizeof...(ReadOnly) > 0, "A query must require at least one component type");
        static_assert(internal::UNIQUE_TYPES<Written..., ReadOnly...>, "A query cannot contain the same component type more than once");

        Query(const Query&) = delete;

        Query& operator=(const Query&) = delete;

        Query(Query&& other) noexcept : m_source(other.m_source), m_revision(other.m_revision), m_matches(std::move(other.m_matches)) {
            other.m_source = nullptr;
        }

        Query& operator=(Query&& other) noexcept {
            if (this == &other) {
                return *this;
            }
            m_source = other.m_source;
            m_revision = other.m_revision;
            m_matches = std::move(other.m_matches);
            other.m_source = nullptr;
            return *this;
        }

        template<typename Function>
        void each(Function&& function) {
            TE_CHECK(m_source != nullptr, "Cannot iterate a moved-from query");
            refreshMatches();
            const IterationGuard guard(*m_source);

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
        template<typename Function>
        static void forEachWrittenType(Function&& function) {
            (function(componentTypeId<Written>()), ...);
        }

        template<typename Function>
        static void forEachReadOnlyType(Function&& function) {
            (function(componentTypeId<ReadOnly>()), ...);
        }

        void refreshMatches() {
            if (m_revision == m_source->queryRevision()) {
                return;
            }

            m_matches.clear();
            for (std::size_t archetypeIndex = 0; archetypeIndex < m_source->queryArchetypeCount(); archetypeIndex++) {
                const bool matches = (m_source->queryArchetypeContains(archetypeIndex, componentTypeId<Written>()) && ...) && (m_source->queryArchetypeContains(archetypeIndex, componentTypeId<ReadOnly>()) && ...);
                if (!matches) {
                    continue;
                }

                m_matches.push_back({m_source->queryEntities(archetypeIndex), {m_source->queryColumn(archetypeIndex, componentTypeId<Written>())...}, {m_source->queryColumn(archetypeIndex, componentTypeId<ReadOnly>())...}});
            }
            m_revision = m_source->queryRevision();
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
