#include <TechEngine/core/scene/ComponentRegistry.hpp>

#include <scene/Archetype.hpp>
#include <scene/ArchetypeStorage.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <unordered_set>

struct ArchetypePosition {
    int value = 0;
};

struct ArchetypeVelocity {
    int value = 0;
};

struct ArchetypeHealth {
    int value = 0;
};

template<typename T>
static void requireColumnMatchesLocations(const TechEngine::ArchetypeStorage& storage, const TechEngine::Archetype& archetype, const TechEngine::ComponentDenseId type) {
    const std::span<const TechEngine::Entity> entities = archetype.entities();
    const std::span<const T> components = archetype.components<T>(type);

    REQUIRE(components.size() == entities.size());
    for (std::size_t row = 0; row < entities.size(); row++) {
        const auto* location = storage.location(entities[row]);
        REQUIRE(location != nullptr);
        REQUIRE(location->archetype == &archetype);
        REQUIRE(location->row == row);
        REQUIRE(storage.component<T>(entities[row]) == &components[row]);
    }
}

TEST_CASE("an empty archetype has no signature columns or rows", "[core][scene][archetype]") {
    TechEngine::ComponentRegistry registry;
    const TechEngine::Archetype archetype({}, registry);

    REQUIRE(archetype.signature().empty());
    REQUIRE(archetype.entities().empty());
    REQUIRE_FALSE(archetype.contains(TechEngine::ComponentDenseId{0}));
    REQUIRE(archetype.rowCount() == 0);
    REQUIRE(archetype.rowCountsMatch());
}

TEST_CASE("an archetype creates one typed column for each signature entry", "[core][scene][archetype]") {
    TechEngine::ComponentRegistry registry;
    const TechEngine::ComponentTypeId positionId = registry.registerComponent<ArchetypePosition>("Tests.ArchetypePosition");
    const TechEngine::ComponentTypeId velocityId = registry.registerComponent<ArchetypeVelocity>("Tests.ArchetypeVelocity");
    const TechEngine::ComponentTypeId healthId = registry.registerComponent<ArchetypeHealth>("Tests.ArchetypeHealth");
    const TechEngine::ComponentDenseId position = registry.find(positionId)->denseId;
    const TechEngine::ComponentDenseId velocity = registry.find(velocityId)->denseId;
    const TechEngine::ComponentDenseId health = registry.find(healthId)->denseId;
    const TechEngine::Archetype archetype({position, health}, registry);

    REQUIRE(archetype.signature().size() == 2);
    REQUIRE(archetype.signature()[0] == position);
    REQUIRE(archetype.signature()[1] == health);
    REQUIRE(archetype.contains(position));
    REQUIRE_FALSE(archetype.contains(velocity));
    REQUIRE(archetype.contains(health));
    REQUIRE(archetype.components<ArchetypePosition>(position).empty());
    REQUIRE(archetype.components<ArchetypeHealth>(health).empty());
    REQUIRE(archetype.entities().empty());
    REQUIRE(archetype.rowCount() == 0);
    REQUIRE(archetype.rowCountsMatch());
}

TEST_CASE("component insertion order converges on one canonical archetype", "[core][scene][archetype]") {
    TechEngine::ComponentRegistry registry;
    const TechEngine::ComponentTypeId positionId = registry.registerComponent<ArchetypePosition>("Tests.ArchetypePosition");
    const TechEngine::ComponentTypeId velocityId = registry.registerComponent<ArchetypeVelocity>("Tests.ArchetypeVelocity");
    const TechEngine::ComponentTypeId healthId = registry.registerComponent<ArchetypeHealth>("Tests.ArchetypeHealth");
    const TechEngine::ComponentDenseId position = registry.find(positionId)->denseId;
    const TechEngine::ComponentDenseId velocity = registry.find(velocityId)->denseId;
    const TechEngine::ComponentDenseId health = registry.find(healthId)->denseId;
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity first = storage.createEntity();
    const TechEngine::Entity second = storage.createEntity();
    const TechEngine::Entity third = storage.createEntity();

    REQUIRE(storage.addComponent(first, ArchetypeHealth{13}));
    REQUIRE(storage.addComponent(first, ArchetypePosition{11}));
    REQUIRE(storage.addComponent(first, ArchetypeVelocity{12}));
    REQUIRE(storage.addComponent(second, ArchetypeVelocity{22}));
    REQUIRE(storage.addComponent(second, ArchetypeHealth{23}));
    REQUIRE(storage.addComponent(second, ArchetypePosition{21}));
    REQUIRE(storage.addComponent(third, ArchetypePosition{31}));
    REQUIRE(storage.addComponent(third, ArchetypeVelocity{32}));
    REQUIRE(storage.addComponent(third, ArchetypeHealth{33}));
    const TechEngine::Archetype* archetype = storage.location(first)->archetype;

    REQUIRE(archetype == storage.location(second)->archetype);
    REQUIRE(archetype == storage.location(third)->archetype);
    REQUIRE(archetype->signature().size() == 3);
    REQUIRE(archetype->signature()[0] == position);
    REQUIRE(archetype->signature()[1] == velocity);
    REQUIRE(archetype->signature()[2] == health);
    REQUIRE(archetype->contains(position));
    REQUIRE(archetype->contains(velocity));
    REQUIRE(archetype->contains(health));
    REQUIRE(archetype->rowCount() == 3);
    REQUIRE(archetype->rowCountsMatch());
    requireColumnMatchesLocations<ArchetypePosition>(storage, *archetype, position);
    requireColumnMatchesLocations<ArchetypeVelocity>(storage, *archetype, velocity);
    requireColumnMatchesLocations<ArchetypeHealth>(storage, *archetype, health);
    REQUIRE(storage.component<ArchetypePosition>(first)->value == 11);
    REQUIRE(storage.component<ArchetypeVelocity>(first)->value == 12);
    REQUIRE(storage.component<ArchetypeHealth>(first)->value == 13);
    REQUIRE(storage.component<ArchetypePosition>(second)->value == 21);
    REQUIRE(storage.component<ArchetypeVelocity>(second)->value == 22);
    REQUIRE(storage.component<ArchetypeHealth>(second)->value == 23);
    REQUIRE(storage.component<ArchetypePosition>(third)->value == 31);
    REQUIRE(storage.component<ArchetypeVelocity>(third)->value == 32);
    REQUIRE(storage.component<ArchetypeHealth>(third)->value == 33);
}

TEST_CASE("failed component mutations leave the archetype row unchanged", "[core][scene][archetype]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<ArchetypePosition>("Tests.ArchetypePosition");
    registry.registerComponent<ArchetypeVelocity>("Tests.ArchetypeVelocity");
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity entity = storage.createEntity();
    REQUIRE(storage.addComponent(entity, ArchetypePosition{17}));
    const TechEngine::Archetype* archetype = storage.location(entity)->archetype;
    const std::size_t row = storage.location(entity)->row;
    const std::size_t archetypeCount = storage.archetypeCount();

    REQUIRE_FALSE(storage.addComponent(entity, ArchetypePosition{99}));
    REQUIRE_FALSE(storage.removeComponent<ArchetypeVelocity>(entity));

    REQUIRE(storage.location(entity)->archetype == archetype);
    REQUIRE(storage.location(entity)->row == row);
    REQUIRE(storage.archetypeCount() == archetypeCount);
    REQUIRE(storage.component<ArchetypePosition>(entity)->value == 17);
    REQUIRE(archetype->rowCount() == 1);
    REQUIRE(archetype->rowCountsMatch());
}

TEST_CASE("transition cycles reuse the same archetype instances", "[core][scene][archetype]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<ArchetypePosition>("Tests.ArchetypePosition");
    registry.registerComponent<ArchetypeVelocity>("Tests.ArchetypeVelocity");
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity entity = storage.createEntity();
    const TechEngine::Archetype* emptyArchetype = storage.location(entity)->archetype;
    REQUIRE(storage.addComponent(entity, ArchetypePosition{41}));
    const TechEngine::Archetype* positionArchetype = storage.location(entity)->archetype;
    REQUIRE(storage.addComponent(entity, ArchetypeVelocity{42}));
    const TechEngine::Archetype* fullArchetype = storage.location(entity)->archetype;

    for (int iteration = 0; iteration < 8; iteration++) {
        REQUIRE(storage.removeComponent<ArchetypeVelocity>(entity));
        REQUIRE(storage.location(entity)->archetype == positionArchetype);
        REQUIRE(storage.component<ArchetypePosition>(entity)->value == 41);
        REQUIRE(storage.addComponent(entity, ArchetypeVelocity{iteration}));
        REQUIRE(storage.location(entity)->archetype == fullArchetype);
        REQUIRE(storage.component<ArchetypePosition>(entity)->value == 41);
        REQUIRE(storage.component<ArchetypeVelocity>(entity)->value == iteration);
    }

    REQUIRE(storage.removeComponent<ArchetypeVelocity>(entity));
    REQUIRE(storage.removeComponent<ArchetypePosition>(entity));
    REQUIRE(storage.location(entity)->archetype == emptyArchetype);
    REQUIRE(storage.archetypeCount() == 3);
    REQUIRE(emptyArchetype->rowCountsMatch());
    REQUIRE(positionArchetype->rowCountsMatch());
    REQUIRE(fullArchetype->rowCountsMatch());
}

TEST_CASE("archetype entity and component rows stay parallel", "[core][scene][archetype]") {
    TechEngine::ComponentRegistry registry;
    const TechEngine::ComponentTypeId positionId = registry.registerComponent<ArchetypePosition>("Tests.ArchetypePosition");
    const TechEngine::ComponentDenseId position = registry.find(positionId)->denseId;
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity first = storage.createEntity();
    const TechEngine::Entity middle = storage.createEntity();
    const TechEngine::Entity last = storage.createEntity();
    REQUIRE(storage.addComponent(first, ArchetypePosition{10}));
    REQUIRE(storage.addComponent(middle, ArchetypePosition{20}));
    REQUIRE(storage.addComponent(last, ArchetypePosition{30}));
    const TechEngine::Archetype* archetype = storage.location(first)->archetype;
    const std::span<const TechEngine::Entity> entities = archetype->entities();
    const std::span<const ArchetypePosition> positions = archetype->components<ArchetypePosition>(position);

    REQUIRE(archetype == storage.location(middle)->archetype);
    REQUIRE(archetype == storage.location(last)->archetype);
    REQUIRE(entities.size() == positions.size());
    REQUIRE(entities.size() == 3);
    for (std::size_t row = 0; row < entities.size(); row++) {
        REQUIRE(storage.component<ArchetypePosition>(entities[row])->value == positions[row].value);
        REQUIRE(storage.location(entities[row])->row == row);
    }
    REQUIRE(archetype->rowCountsMatch());
}

TEST_CASE("archetype swap removal moves the tail entity and every column together", "[core][scene][archetype]") {
    TechEngine::ComponentRegistry registry;
    const TechEngine::ComponentTypeId positionId = registry.registerComponent<ArchetypePosition>("Tests.ArchetypePosition");
    const TechEngine::ComponentTypeId velocityId = registry.registerComponent<ArchetypeVelocity>("Tests.ArchetypeVelocity");
    const TechEngine::ComponentDenseId position = registry.find(positionId)->denseId;
    const TechEngine::ComponentDenseId velocity = registry.find(velocityId)->denseId;
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity first = storage.createEntity();
    const TechEngine::Entity middle = storage.createEntity();
    const TechEngine::Entity last = storage.createEntity();

    const std::array entities = {first, middle, last};
    for (const TechEngine::Entity entity: entities) {
        REQUIRE(storage.addComponent(entity, ArchetypePosition{static_cast<int>(entity.index) + 10}));
        REQUIRE(storage.addComponent(entity, ArchetypeVelocity{static_cast<int>(entity.index) + 20}));
    }

    const TechEngine::Archetype* archetype = storage.location(middle)->archetype;
    const std::size_t removedRow = storage.location(middle)->row;
    REQUIRE(storage.destroyEntity(middle));

    REQUIRE(archetype->entities()[removedRow] == last);
    REQUIRE(archetype->components<ArchetypePosition>(position)[removedRow].value == static_cast<int>(last.index) + 10);
    REQUIRE(archetype->components<ArchetypeVelocity>(velocity)[removedRow].value == static_cast<int>(last.index) + 20);
    REQUIRE(archetype->rowCount() == 2);
    REQUIRE(archetype->rowCountsMatch());
}

TEST_CASE("removing an archetype's only row empties every column", "[core][scene][archetype]") {
    TechEngine::ComponentRegistry registry;
    const TechEngine::ComponentTypeId positionId = registry.registerComponent<ArchetypePosition>("Tests.ArchetypePosition");
    const TechEngine::ComponentTypeId velocityId = registry.registerComponent<ArchetypeVelocity>("Tests.ArchetypeVelocity");
    const TechEngine::ComponentDenseId position = registry.find(positionId)->denseId;
    const TechEngine::ComponentDenseId velocity = registry.find(velocityId)->denseId;
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity entity = storage.createEntity();
    REQUIRE(storage.addComponent(entity, ArchetypePosition{4}));
    REQUIRE(storage.addComponent(entity, ArchetypeVelocity{8}));
    const TechEngine::Archetype* archetype = storage.location(entity)->archetype;

    REQUIRE(storage.destroyEntity(entity));

    REQUIRE(archetype->entities().empty());
    REQUIRE(archetype->components<ArchetypePosition>(position).empty());
    REQUIRE(archetype->components<ArchetypeVelocity>(velocity).empty());
    REQUIRE(archetype->rowCount() == 0);
    REQUIRE(archetype->rowCountsMatch());
}

TEST_CASE("mixed migrations and removals preserve every reachable archetype invariant", "[core][scene][archetype]") {
    TechEngine::ComponentRegistry registry;
    const TechEngine::ComponentTypeId positionId = registry.registerComponent<ArchetypePosition>("Tests.ArchetypePosition");
    const TechEngine::ComponentTypeId velocityId = registry.registerComponent<ArchetypeVelocity>("Tests.ArchetypeVelocity");
    const TechEngine::ComponentTypeId healthId = registry.registerComponent<ArchetypeHealth>("Tests.ArchetypeHealth");
    const TechEngine::ComponentDenseId position = registry.find(positionId)->denseId;
    const TechEngine::ComponentDenseId velocity = registry.find(velocityId)->denseId;
    const TechEngine::ComponentDenseId health = registry.find(healthId)->denseId;
    TechEngine::ArchetypeStorage storage(registry);
    std::array<TechEngine::Entity, 12> entities{};
    std::array<bool, 12> alive{};
    alive.fill(true);

    for (std::size_t index = 0; index < entities.size(); index++) {
        entities[index] = storage.createEntity();
        REQUIRE(storage.addComponent(entities[index], ArchetypePosition{static_cast<int>(index) + 10}));
        if (index % 2U == 0) {
            REQUIRE(storage.addComponent(entities[index], ArchetypeVelocity{static_cast<int>(index) + 20}));
        }
        if (index % 3U == 0) {
            REQUIRE(storage.addComponent(entities[index], ArchetypeHealth{static_cast<int>(index) + 30}));
        }
    }

    for (std::size_t index = 0; index < entities.size(); index++) {
        if (index % 2U == 0) {
            REQUIRE(storage.removeComponent<ArchetypeVelocity>(entities[index]));
            if (index % 3U != 0) {
                REQUIRE(storage.addComponent(entities[index], ArchetypeHealth{static_cast<int>(index) + 40}));
            }
        } else {
            REQUIRE(storage.removeComponent<ArchetypePosition>(entities[index]));
            REQUIRE(storage.addComponent(entities[index], ArchetypeVelocity{static_cast<int>(index) + 50}));
        }
    }

    for (const std::size_t index: std::array<std::size_t, 3>{1, 6, 10}) {
        REQUIRE(storage.destroyEntity(entities[index]));
        alive[index] = false;
    }

    std::unordered_set<const TechEngine::Archetype*> checkedArchetypes;
    for (std::size_t index = 0; index < entities.size(); index++) {
        if (!alive[index]) {
            REQUIRE(storage.location(entities[index]) == nullptr);
            continue;
        }

        const auto* location = storage.location(entities[index]);
        REQUIRE(location != nullptr);
        const TechEngine::Archetype& archetype = *location->archetype;
        REQUIRE(location->row < archetype.rowCount());
        REQUIRE(archetype.entities()[location->row] == entities[index]);
        REQUIRE(archetype.rowCountsMatch());
        REQUIRE(std::ranges::is_sorted(archetype.signature()));
        REQUIRE(std::ranges::adjacent_find(archetype.signature()) == archetype.signature().end());

        if (index % 2U == 0) {
            REQUIRE(storage.component<ArchetypePosition>(entities[index]) != nullptr);
            REQUIRE(storage.component<ArchetypePosition>(entities[index])->value == static_cast<int>(index) + 10);
            REQUIRE(storage.component<ArchetypeVelocity>(entities[index]) == nullptr);
            const int expectedHealth = index % 3U == 0 ? static_cast<int>(index) + 30 : static_cast<int>(index) + 40;
            REQUIRE(storage.component<ArchetypeHealth>(entities[index]) != nullptr);
            REQUIRE(storage.component<ArchetypeHealth>(entities[index])->value == expectedHealth);
        } else {
            REQUIRE(storage.component<ArchetypePosition>(entities[index]) == nullptr);
            REQUIRE(storage.component<ArchetypeVelocity>(entities[index]) != nullptr);
            REQUIRE(storage.component<ArchetypeVelocity>(entities[index])->value == static_cast<int>(index) + 50);
            if (index % 3U == 0) {
                REQUIRE(storage.component<ArchetypeHealth>(entities[index]) != nullptr);
                REQUIRE(storage.component<ArchetypeHealth>(entities[index])->value == static_cast<int>(index) + 30);
            } else {
                REQUIRE(storage.component<ArchetypeHealth>(entities[index]) == nullptr);
            }
        }

        if (checkedArchetypes.insert(&archetype).second) {
            if (archetype.contains(position)) {
                requireColumnMatchesLocations<ArchetypePosition>(storage, archetype, position);
            }
            if (archetype.contains(velocity)) {
                requireColumnMatchesLocations<ArchetypeVelocity>(storage, archetype, velocity);
            }
            if (archetype.contains(health)) {
                requireColumnMatchesLocations<ArchetypeHealth>(storage, archetype, health);
            }
        }
    }
    REQUIRE(checkedArchetypes.size() == 3);
}
