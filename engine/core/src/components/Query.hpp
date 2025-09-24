#pragma once

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

    public:
        template<typename Func>
        void each(Func function) {
            for (Archetype* archetype: m_matchingArchetypes) {
                // This is the special case: user wants the Entity ID.
                if constexpr (std::is_same_v<std::tuple_element_t<0, std::tuple<Components...>>, Entity>) {
                    // We use our helper to get a type_pack of all components *except* the first one (Entity).
                    using SlicedComponents = pop_front<type_pack<Components...>>::type;
                    // Now, we execute the loop with this sliced pack.
                    each_impl(archetype, function, SlicedComponents{});
                } else {
                    // Normal case: user just wants components.
                    each_impl(archetype, function, type_pack<Components...>{});
                }
            }
        }

    private:
        template<typename Func, typename... ComponentTypes>
        void each_impl(Archetype* archetype, Func function, type_pack<ComponentTypes...>) {
            auto component_arrays = std::make_tuple(
                archetype->getComponentArrayAsRawPtr<ComponentTypes>()...
            );
            size_t entity_count = archetype->getEntities().size();

            for (size_t i = 0; i < entity_count; ++i) {
                // We use another helper to correctly unpack and call.
                call_with_components(function, component_arrays, archetype->getEntities()[i], i, std::index_sequence_for<ComponentTypes...>{});
            }
        }

        template<typename Func, typename Tuple, size_t... Is>
        void call_with_components(Func function, Tuple& arrays, Entity entity, size_t index, std::index_sequence<Is...>) {
            // This is the special case: if the first template argument is Entity, pass it.
            if constexpr (std::is_same_v<std::tuple_element_t<0, std::tuple<Components...>>, Entity>) {
                // We pass the entity ID, then unpack the rest of the component references.
                function(entity, std::get<Is>(arrays)[index]...);
            } else {
                // Otherwise, just unpack the component references.
                function(std::get<Is>(arrays)[index]...);
            }
        }
    };
}
