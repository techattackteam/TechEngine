#pragma once
#include <future>
#include <shared_mutex>

namespace TechEngine {
    template<typename... Ts>
    struct type_pack {
    };

    template<typename T, typename... Ts>
    struct pop_front;

    template<typename T, typename... Ts>
    struct pop_front<type_pack<T, Ts...>> {
        using type = type_pack<Ts...>;
    };

    template<typename... Components>
    class Query {
    private:
        friend class ArchetypesManager; // Allow ECS to create and populate Query objects

        // Private constructor so only ECS can create queries
        explicit Query(ArchetypesManager* ecs) : ecs_(ecs) {
        }

        ArchetypesManager* ecs_;
        std::vector<Archetype*> m_matchingArchetypes;
        mutable std::shared_mutex m_sharedMutex;

    public:
        Query(const Query&) = delete;

        Query& operator=(const Query&) = delete;

        Query(Query&& other) noexcept
            : ecs_(other.ecs_),
              m_matchingArchetypes(std::move(other.m_matchingArchetypes)) {
            other.ecs_ = nullptr; // Invalidate the moved-from object
        }


        template<typename Func>
        void each(Func function) {
            for (Archetype* archetype: m_matchingArchetypes) {
                // This is the special case: user wants the Entity ID.
                if constexpr (std::is_same_v<std::tuple_element_t<0, std::tuple<Components...>>, Entity>) {
                    // I use our helper to get a type_pack of all components *except* the first one (Entity).
                    using SlicedComponents = pop_front<type_pack<Components...>>::type;
                    // Now, I execute the loop with this sliced pack.
                    eachImpl(archetype, function, SlicedComponents{});
                } else {
                    // Normal case: user just wants components.
                    eachImpl(archetype, function, type_pack<Components...>{});
                }
            }
        }

        template<typename Func>
        void parallelEach(Func function) {
            for (Archetype* archetype: m_matchingArchetypes) {
                // This is the special case: user wants the Entity ID.
                if constexpr (std::is_same_v<std::tuple_element_t<0, std::tuple<Components...>>, Entity>) {
                    // I use our helper to get a type_pack of all components *except* the first one (Entity).
                    using SlicedComponents = pop_front<type_pack<Components...>>::type;
                    // Now, I execute the loop with this sliced pack.
                    parallelEachImpl(archetype, function, SlicedComponents{});
                } else {
                    // Normal case: user just wants components.
                    parallelEachImpl(archetype, function, type_pack<Components...>{});
                }
            }
        }

    private:
        template<typename Func, typename... ComponentTypes>
        void eachImpl(Archetype* archetype, Func function, type_pack<ComponentTypes...>) {
            auto componentArrays = std::make_tuple(
                archetype->getComponentArrayAsRawPtr<ComponentTypes>()...
            );
            size_t entityCount = archetype->getEntities().size();

            for (size_t i = 0; i < entityCount; ++i) {
                // I use another helper to correctly unpack and call.
                callWithComponents(function, componentArrays, archetype->getEntities()[i], i, std::index_sequence_for<ComponentTypes...>{});
            }
        }

        template<typename Func, typename... ComponentTypes>
        void parallelEachImpl(Archetype* archetype, Func function, type_pack<ComponentTypes...>) {
            auto componentArrays = std::make_tuple(
                archetype->getComponentArrayAsRawPtr<ComponentTypes>()...
            );
            const unsigned int numThreads = std::thread::hardware_concurrency();
            std::vector<std::thread> threads;
            threads.reserve(numThreads);
            size_t entityCount = archetype->getEntities().size();

            const size_t chunkSize = (entityCount + numThreads - 1) / numThreads;
            for (unsigned int t = 0; t < numThreads; ++t) {
                size_t start = t * chunkSize;
                size_t end = std::min(start + chunkSize, entityCount);

                if (start >= end) {
                    continue;
                }

                threads.emplace_back([this, &function, &componentArrays, archetype, start, end]() {
                    for (size_t i = start; i < end; ++i) {
                        parallelCallWithComponents(function, componentArrays, archetype->getEntities()[i], i, std::index_sequence_for<ComponentTypes...>{});
                    }
                });
            }
            for (auto& thread: threads) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
        }

        template<typename Func, typename Tuple, size_t... Is>
        void callWithComponents(Func function, Tuple& arrays, Entity entity, size_t index, std::index_sequence<Is...>) {
            if constexpr (std::is_same_v<std::tuple_element_t<0, std::tuple<Components...>>, Entity>) {
                function(entity, std::get<Is>(arrays)[index]...);
            } else {
                function(std::get<Is>(arrays)[index]...);
            }
        }

        template<typename Func, typename Tuple, size_t... Is>
        void parallelCallWithComponents(Func function, Tuple& arrays, Entity entity, size_t index, std::index_sequence<Is...>) {
            if constexpr (std::is_same_v<std::tuple_element_t<0, std::tuple<Components...>>, Entity>) {
                function(m_sharedMutex, entity, std::get<Is>(arrays)[index]...);
            } else {
                function(m_sharedMutex, std::get<Is>(arrays)[index]...);
            }
        }
    };
}
