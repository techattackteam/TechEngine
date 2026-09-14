#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/ComponentStorage.hpp>

#include <scene/ArchetypeStorage.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <memory>
#include <span>
#include <stdexcept>

struct StoragePosition {
    int value = 0;
};

struct StorageVelocity {
    int value = 0;
};

struct StorageWithoutDefault {
    explicit StorageWithoutDefault(int initial) : value(initial) {
    }

    int value = 0;
};

struct StorageWithThrowingMove {
    StorageWithThrowingMove() = default;
    StorageWithThrowingMove(const StorageWithThrowingMove&) = default;
    StorageWithThrowingMove(StorageWithThrowingMove&& other) noexcept(false) : value(other.value) {
    }
    StorageWithThrowingMove& operator=(const StorageWithThrowingMove&) = default;
    StorageWithThrowingMove& operator=(StorageWithThrowingMove&& other) noexcept(false) {
        value = other.value;
        return *this;
    }

    int value = 0;
};

struct StorageWithoutCopy {
    StorageWithoutCopy() = default;
    StorageWithoutCopy(const StorageWithoutCopy&) = delete;
    StorageWithoutCopy(StorageWithoutCopy&&) noexcept = default;
    StorageWithoutCopy& operator=(const StorageWithoutCopy&) = delete;
    StorageWithoutCopy& operator=(StorageWithoutCopy&&) noexcept = default;
};

struct StorageWithThrowingDefault {
    inline static bool throwOnDefault = false;

    StorageWithThrowingDefault() {
        if (throwOnDefault) {
            throw std::runtime_error("default construction failed");
        }
    }

    explicit StorageWithThrowingDefault(const int initial) : value(initial) {
    }

    StorageWithThrowingDefault(const StorageWithThrowingDefault&) = default;
    StorageWithThrowingDefault(StorageWithThrowingDefault&&) noexcept = default;
    StorageWithThrowingDefault& operator=(const StorageWithThrowingDefault&) = default;
    StorageWithThrowingDefault& operator=(StorageWithThrowingDefault&&) noexcept = default;

    int value = 0;
};

struct StorageWithThrowingCopy {
    inline static bool throwOnCopy = false;

    StorageWithThrowingCopy() = default;

    explicit StorageWithThrowingCopy(const int initial) : value(initial) {
    }

    StorageWithThrowingCopy(const StorageWithThrowingCopy& other) : value(other.value) {
        if (throwOnCopy) {
            throw std::runtime_error("copy construction failed");
        }
    }

    StorageWithThrowingCopy(StorageWithThrowingCopy&&) noexcept = default;

    StorageWithThrowingCopy& operator=(const StorageWithThrowingCopy& other) {
        if (throwOnCopy) {
            throw std::runtime_error("copy assignment failed");
        }
        value = other.value;
        return *this;
    }

    StorageWithThrowingCopy& operator=(StorageWithThrowingCopy&&) noexcept = default;

    int value = 0;
};

struct StorageLifetime {
    inline static int liveCount = 0;

    StorageLifetime() {
        liveCount++;
    }

    explicit StorageLifetime(const int initial) : value(initial) {
        liveCount++;
    }

    StorageLifetime(const StorageLifetime& other) : value(other.value) {
        liveCount++;
    }

    StorageLifetime(StorageLifetime&& other) noexcept : value(other.value) {
        liveCount++;
    }

    StorageLifetime& operator=(const StorageLifetime&) = default;
    StorageLifetime& operator=(StorageLifetime&&) noexcept = default;

    ~StorageLifetime() {
        liveCount--;
    }

    int value = 0;
};

class ArchetypeStorageThrowFlagGuard {
private:
    bool* m_value = nullptr;

public:
    explicit ArchetypeStorageThrowFlagGuard(bool& value) : m_value(&value) {
        value = true;
    }

    ~ArchetypeStorageThrowFlagGuard() {
        *m_value = false;
    }

    ArchetypeStorageThrowFlagGuard(const ArchetypeStorageThrowFlagGuard&) = delete;

    ArchetypeStorageThrowFlagGuard& operator=(const ArchetypeStorageThrowFlagGuard&) = delete;
};

static_assert(TechEngine::ComponentValue<StoragePosition>);
static_assert(!TechEngine::ComponentValue<StorageWithoutDefault>);
static_assert(!TechEngine::ComponentValue<StorageWithThrowingMove>);
static_assert(!TechEngine::ComponentValue<StorageWithoutCopy>);
static_assert(TechEngine::ComponentValue<StorageWithThrowingDefault>);
static_assert(TechEngine::ComponentValue<StorageWithThrowingCopy>);
static_assert(TechEngine::ComponentValue<StorageLifetime>);

static std::size_t collideSignatures(std::span<const TechEngine::ComponentDenseId>) {
    return 1;
}

TEST_CASE("component registration supplies typed vector storage", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    const TechEngine::ComponentTypeId positionId = registry.registerComponent<StoragePosition>("Tests.StoragePosition");
    const TechEngine::ComponentTypeRecord* record = registry.find(positionId);

    REQUIRE(record != nullptr);
    std::unique_ptr<TechEngine::IComponentStorage> storage = record->createStorage();
    storage->appendDefault();
    auto& typed = static_cast<TechEngine::ComponentStorage<StoragePosition>&>(*storage);
    typed.set(0, {42});

    REQUIRE(storage->size() == 1);
    REQUIRE(typed.values()[0].value == 42);
}

TEST_CASE("component transitions preserve shared columns", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<StoragePosition>("Tests.StoragePosition");
    registry.registerComponent<StorageVelocity>("Tests.StorageVelocity");
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity entity = storage.createEntity();

    REQUIRE(storage.addComponent(entity, StoragePosition{17}));
    REQUIRE(storage.addComponent(entity, StorageVelocity{3}));
    REQUIRE(storage.component<StoragePosition>(entity)->value == 17);
    REQUIRE(storage.component<StorageVelocity>(entity)->value == 3);

    REQUIRE(storage.removeComponent<StorageVelocity>(entity));
    REQUIRE(storage.component<StoragePosition>(entity)->value == 17);
    REQUIRE(storage.component<StorageVelocity>(entity) == nullptr);

    REQUIRE(storage.addComponent(entity, StorageVelocity{8}));
    REQUIRE(storage.component<StoragePosition>(entity)->value == 17);
    REQUIRE(storage.component<StorageVelocity>(entity)->value == 8);
    REQUIRE(storage.archetypeCount() == 3);
}

TEST_CASE("removing first middle and last rows preserves locations and values", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<StoragePosition>("Tests.StoragePosition");
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity first = storage.createEntity();
    const TechEngine::Entity middle = storage.createEntity();
    const TechEngine::Entity last = storage.createEntity();
    REQUIRE(storage.addComponent(first, StoragePosition{1}));
    REQUIRE(storage.addComponent(middle, StoragePosition{2}));
    REQUIRE(storage.addComponent(last, StoragePosition{3}));

    SECTION("first") {
        const std::size_t removedRow = storage.location(first)->row;
        REQUIRE(storage.destroyEntity(first));

        REQUIRE_FALSE(storage.contains(first));
        REQUIRE(storage.location(last)->row == removedRow);
        REQUIRE(storage.component<StoragePosition>(last)->value == 3);
        REQUIRE(storage.component<StoragePosition>(middle)->value == 2);
        REQUIRE(storage.location(last)->archetype->rowCountsMatch());
    }

    SECTION("middle") {
        const std::size_t removedRow = storage.location(middle)->row;
        REQUIRE(storage.destroyEntity(middle));

        REQUIRE_FALSE(storage.contains(middle));
        REQUIRE(storage.location(last)->row == removedRow);
        REQUIRE(storage.component<StoragePosition>(last)->value == 3);
        REQUIRE(storage.component<StoragePosition>(first)->value == 1);
        REQUIRE(storage.location(last)->archetype->rowCountsMatch());
    }

    SECTION("last") {
        const std::size_t firstRow = storage.location(first)->row;
        const std::size_t middleRow = storage.location(middle)->row;
        REQUIRE(storage.destroyEntity(last));

        REQUIRE_FALSE(storage.contains(last));
        REQUIRE(storage.location(first)->row == firstRow);
        REQUIRE(storage.location(middle)->row == middleRow);
        REQUIRE(storage.component<StoragePosition>(first)->value == 1);
        REQUIRE(storage.component<StoragePosition>(middle)->value == 2);
        REQUIRE(storage.location(first)->archetype->rowCountsMatch());
    }
}

TEST_CASE("a component transition repairs the source archetype location", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<StoragePosition>("Tests.StoragePosition");
    registry.registerComponent<StorageVelocity>("Tests.StorageVelocity");
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity first = storage.createEntity();
    const TechEngine::Entity middle = storage.createEntity();
    const TechEngine::Entity last = storage.createEntity();
    REQUIRE(storage.addComponent(first, StoragePosition{1}));
    REQUIRE(storage.addComponent(middle, StoragePosition{2}));
    REQUIRE(storage.addComponent(last, StoragePosition{3}));

    const std::size_t vacatedRow = storage.location(middle)->row;
    REQUIRE(storage.addComponent(middle, StorageVelocity{4}));

    REQUIRE(storage.location(last)->row == vacatedRow);
    REQUIRE(storage.component<StoragePosition>(last)->value == 3);
    REQUIRE(storage.component<StoragePosition>(middle)->value == 2);
    REQUIRE(storage.component<StorageVelocity>(middle)->value == 4);
    REQUIRE(storage.location(last)->archetype->rowCountsMatch());
    REQUIRE(storage.location(middle)->archetype->rowCountsMatch());
}

TEST_CASE("full signatures distinguish archetypes with colliding hashes", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<StoragePosition>("Tests.StoragePosition");
    registry.registerComponent<StorageVelocity>("Tests.StorageVelocity");
    TechEngine::ArchetypeStorage storage(registry, &collideSignatures);
    const TechEngine::Entity positionEntity = storage.createEntity();
    const TechEngine::Entity velocityEntity = storage.createEntity();

    REQUIRE(storage.addComponent(positionEntity, StoragePosition{4}));
    REQUIRE(storage.addComponent(velocityEntity, StorageVelocity{9}));

    REQUIRE(storage.archetypeCount() == 3);
    REQUIRE(storage.component<StoragePosition>(positionEntity)->value == 4);
    REQUIRE(storage.component<StorageVelocity>(positionEntity) == nullptr);
    REQUIRE(storage.component<StorageVelocity>(velocityEntity)->value == 9);
    REQUIRE(storage.component<StoragePosition>(velocityEntity) == nullptr);
}

TEST_CASE("row counts remain aligned through repeated transitions", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<StoragePosition>("Tests.StoragePosition");
    registry.registerComponent<StorageVelocity>("Tests.StorageVelocity");
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity entity = storage.createEntity();

    for (int iteration = 0; iteration < 8; iteration++) {
        REQUIRE(storage.addComponent(entity, StoragePosition{iteration}));
        REQUIRE(storage.location(entity)->archetype->rowCountsMatch());
        REQUIRE(storage.addComponent(entity, StorageVelocity{iteration * 2}));
        REQUIRE(storage.location(entity)->archetype->rowCountsMatch());
        REQUIRE(storage.removeComponent<StoragePosition>(entity));
        REQUIRE(storage.location(entity)->archetype->rowCountsMatch());
        REQUIRE(storage.removeComponent<StorageVelocity>(entity));
        REQUIRE(storage.location(entity)->archetype->rowCountsMatch());
    }
}

TEST_CASE("clearing storage invalidates handles and transition caches", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<StoragePosition>("Tests.StoragePosition");
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity stale = storage.createEntity();
    REQUIRE(storage.addComponent(stale, StoragePosition{6}));

    storage.clear();
    const TechEngine::Entity current = storage.createEntity();

    REQUIRE_FALSE(storage.contains(stale));
    REQUIRE(storage.component<StoragePosition>(stale) == nullptr);
    REQUIRE(current.index == stale.index);
    REQUIRE(current.generation > stale.generation);
    REQUIRE(storage.archetypeCount() == 1);
}

TEST_CASE("throwing default construction rolls back an archetype append", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<StoragePosition>("Tests.StoragePosition");
    registry.registerComponent<StorageWithThrowingDefault>("Tests.StorageWithThrowingDefault");
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity entity = storage.createEntity();
    REQUIRE(storage.addComponent(entity, StoragePosition{12}));
    const StorageWithThrowingDefault value(7);

    {
        const ArchetypeStorageThrowFlagGuard guard(StorageWithThrowingDefault::throwOnDefault);
        REQUIRE_THROWS_AS(storage.addComponent(entity, value), std::runtime_error);
    }

    REQUIRE(storage.component<StoragePosition>(entity)->value == 12);
    REQUIRE(storage.component<StorageWithThrowingDefault>(entity) == nullptr);
    REQUIRE(storage.location(entity)->archetype->rowCountsMatch());

    REQUIRE(storage.addComponent(entity, value));
    REQUIRE(storage.component<StorageWithThrowingDefault>(entity)->value == 7);
}

TEST_CASE("throwing shared-column copy rolls back a component transition", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    registry.registerComponent<StorageWithThrowingCopy>("Tests.StorageWithThrowingCopy");
    registry.registerComponent<StorageVelocity>("Tests.StorageVelocity");
    TechEngine::ArchetypeStorage storage(registry);
    const TechEngine::Entity entity = storage.createEntity();
    REQUIRE(storage.addComponent(entity, StorageWithThrowingCopy(5)));

    {
        const ArchetypeStorageThrowFlagGuard guard(StorageWithThrowingCopy::throwOnCopy);
        REQUIRE_THROWS_AS(storage.addComponent(entity, StorageVelocity{3}), std::runtime_error);
    }

    REQUIRE(storage.component<StorageWithThrowingCopy>(entity)->value == 5);
    REQUIRE(storage.component<StorageVelocity>(entity) == nullptr);
    REQUIRE(storage.location(entity)->archetype->rowCountsMatch());

    REQUIRE(storage.addComponent(entity, StorageVelocity{3}));
    REQUIRE(storage.component<StorageWithThrowingCopy>(entity)->value == 5);
    REQUIRE(storage.component<StorageVelocity>(entity)->value == 3);
}

TEST_CASE("non-trivial component lifetimes balance across transitions", "[core][scene]") {
    StorageLifetime::liveCount = 0;
    {
        TechEngine::ComponentRegistry registry;
        registry.registerComponent<StorageLifetime>("Tests.StorageLifetime");
        registry.registerComponent<StoragePosition>("Tests.StoragePosition");
        TechEngine::ArchetypeStorage storage(registry);
        const TechEngine::Entity entity = storage.createEntity();
        const StorageLifetime value(19);

        REQUIRE(storage.addComponent(entity, value));
        REQUIRE(StorageLifetime::liveCount == 2);
        REQUIRE(storage.addComponent(entity, StoragePosition{2}));
        REQUIRE(StorageLifetime::liveCount == 2);
        REQUIRE(storage.removeComponent<StoragePosition>(entity));
        REQUIRE(StorageLifetime::liveCount == 2);
        REQUIRE(storage.removeComponent<StorageLifetime>(entity));
        REQUIRE(StorageLifetime::liveCount == 1);
    }
    REQUIRE(StorageLifetime::liveCount == 0);
}
