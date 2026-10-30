#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/ReferenceVisitor.hpp>
#include <TechEngine/core/serialization/Writer.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <serialization/FieldRecorder.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

struct RegistryPosition {
    float x;
    float y;
    float z;
};

struct RegistryVelocity {
    float x;
    float y;
    float z;
};

struct RegistryTagSharer {
    std::uint32_t value;
};

struct RegistryThirdType {
    std::uint32_t value;
};

struct RegistryAfterFreeze {
    std::uint32_t value;
};

struct RegistryConflictingType {
    std::uint32_t value;
};

struct RegistryVisited {
    std::uint32_t health = 0;
    float speed = 0.0f;

    template<typename Archive>
    friend void visit(Archive& archive, RegistryVisited& value) {
        archive.field("health", value.health);
        archive.field("speed", value.speed);
    }
};

struct RegistryLinked {
    std::uint32_t health = 0;
    TechEngine::Entity target;

    template<typename Archive>
    friend void visit(Archive& archive, RegistryLinked& value) {
        archive.field("health", value.health);
        archive.field("target", value.target);
    }
};

struct RegistryAssignCounted {
    std::uint32_t health = 0;
    int assignments = 0;

    RegistryAssignCounted() = default;

    RegistryAssignCounted(const RegistryAssignCounted&) = default;

    RegistryAssignCounted(RegistryAssignCounted&&) noexcept = default;

    RegistryAssignCounted& operator=(const RegistryAssignCounted& other) {
        health = other.health;
        assignments++;
        return *this;
    }

    RegistryAssignCounted& operator=(RegistryAssignCounted&& other) noexcept {
        health = other.health;
        assignments++;
        return *this;
    }

    template<typename Archive>
    friend void visit(Archive& archive, RegistryAssignCounted& value) {
        archive.field("health", value.health);
    }
};

class RegistryEntityRecorder final : public TechEngine::ReferenceVisitor {
public:
    std::vector<TechEngine::Entity> entities;

    void visitEntity(TechEngine::Entity& value) override {
        entities.push_back(value);
    }

    void visitAsset(TechEngine::ResourceId, TechEngine::ResourceSlotId&) override {
    }
};

TEST_CASE("component registration bridges a stable tag to a dense id", "[core][scene]") {
    TechEngine::ComponentRegistry registry;

    const TechEngine::ComponentTypeId id = registry.registerComponent<RegistryPosition>("TechEngine.Position");
    const TechEngine::ComponentTypeRecord* record = registry.find(id);

    REQUIRE(id == TechEngine::ComponentTypeId{TechEngine::StringId{"TechEngine.Position"}});
    REQUIRE(TechEngine::componentTypeId<RegistryPosition>() == id);
    REQUIRE(record != nullptr);
    REQUIRE(record->denseId == TechEngine::ComponentDenseId{0});
    REQUIRE(record->tag == "TechEngine.Position");
}

TEST_CASE("dense ids are assigned in registration order", "[core][scene]") {
    TechEngine::ComponentRegistry registry;

    const TechEngine::ComponentTypeId position = registry.registerComponent<RegistryPosition>("TechEngine.Position");
    const TechEngine::ComponentTypeId velocity = registry.registerComponent<RegistryVelocity>("TechEngine.Velocity");

    REQUIRE(registry.find(position)->denseId == TechEngine::ComponentDenseId{0});
    REQUIRE(registry.find(velocity)->denseId == TechEngine::ComponentDenseId{1});
    REQUIRE(registry.find(TechEngine::ComponentDenseId{1})->id == velocity);
}

TEST_CASE("registration order changes dense ids but not stable ids", "[core][scene]") {
    TechEngine::ComponentRegistry first;
    TechEngine::ComponentRegistry second;

    const TechEngine::ComponentTypeId firstPosition = first.registerComponent<RegistryPosition>("TechEngine.Position");
    first.registerComponent<RegistryVelocity>("TechEngine.Velocity");
    second.registerComponent<RegistryVelocity>("TechEngine.Velocity");
    const TechEngine::ComponentTypeId secondPosition = second.registerComponent<RegistryPosition>("TechEngine.Position");

    REQUIRE(firstPosition == secondPosition);
    REQUIRE(first.find(firstPosition)->denseId == TechEngine::ComponentDenseId{0});
    REQUIRE(second.find(secondPosition)->denseId == TechEngine::ComponentDenseId{1});
}

TEST_CASE("a duplicate tag is rejected without consuming a dense id", "[core][scene]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;

    registry.registerComponent<RegistryPosition>("TechEngine.Position");
    REQUIRE_THROWS_AS(registry.registerComponent<RegistryTagSharer>("TechEngine.Position"), TechEngineTests::AssertFired);
    const TechEngine::ComponentTypeId velocity = registry.registerComponent<RegistryVelocity>("TechEngine.Velocity");

    REQUIRE(registry.typeCount() == 2);
    REQUIRE(registry.find(velocity)->denseId == TechEngine::ComponentDenseId{1});
    REQUIRE_FALSE(TechEngine::componentTypeId<RegistryTagSharer>().valid());
}

TEST_CASE("a component type cannot be registered under conflicting tags", "[core][scene]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;

    const TechEngine::ComponentTypeId original = registry.registerComponent<RegistryConflictingType>("TechEngine.Original");
    REQUIRE_THROWS_AS(registry.registerComponent<RegistryConflictingType>("TechEngine.Conflicting"), TechEngineTests::AssertFired);

    REQUIRE(registry.typeCount() == 1);
    REQUIRE(registry.find(original) != nullptr);
    REQUIRE(registry.find(TechEngine::ComponentTypeId{TechEngine::StringId{"TechEngine.Conflicting"}}) == nullptr);
    REQUIRE(TechEngine::componentTypeId<RegistryConflictingType>() == original);
}

TEST_CASE("registration after the freeze is rejected", "[core][scene]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;

    registry.registerComponent<RegistryPosition>("TechEngine.Position");
    registry.freeze();
    REQUIRE_THROWS_AS(registry.registerComponent<RegistryAfterFreeze>("TechEngine.AfterFreeze"), TechEngineTests::AssertFired);

    REQUIRE(registry.frozen());
    REQUIRE(registry.typeCount() == 1);
    REQUIRE_FALSE(TechEngine::componentTypeId<RegistryAfterFreeze>().valid());
}

TEST_CASE("dense id exhaustion reports a check and leaves the registry unchanged", "[core][scene]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry{2};

    registry.registerComponent<RegistryPosition>("TechEngine.Position");
    registry.registerComponent<RegistryVelocity>("TechEngine.Velocity");
    REQUIRE_THROWS_AS(registry.registerComponent<RegistryThirdType>("TechEngine.Third"), TechEngineTests::AssertFired);

    REQUIRE(registry.typeCount() == 2);
    REQUIRE_FALSE(TechEngine::componentTypeId<RegistryThirdType>().valid());
}

TEST_CASE("a component registry rejects a limit above the dense id capacity", "[core][scene]") {
    const TechEngineTests::FatalAssertGuard guard;

    REQUIRE_THROWS_AS(TechEngine::ComponentRegistry{TechEngine::ComponentDenseId::VALUE_COUNT + 1U}, TechEngineTests::AssertFired);
}

TEST_CASE("a component with a visit is registered with payload adapters bound to it", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    const TechEngine::ComponentTypeId id = registry.registerComponent<RegistryVisited>("TechEngine.Visited");
    const TechEngine::ComponentTypeRecord* record = registry.find(id);
    REQUIRE(record != nullptr);
    REQUIRE(record->writePayload != nullptr);
    REQUIRE(record->readPayload != nullptr);
    REQUIRE(record->visitReferences != nullptr);

    const RegistryVisited written{42, 3.5f};
    std::vector<std::byte> adapterBytes;
    TechEngine::Writer adapterWriter{adapterBytes};
    record->writePayload(adapterWriter, &written);

    std::vector<std::byte> fieldBytes;
    TechEngine::Writer fieldWriter{fieldBytes};
    fieldWriter.write(written.health);
    fieldWriter.write(written.speed);
    REQUIRE(adapterBytes == fieldBytes);

    RegistryVisited read;
    TechEngine::Reader reader{adapterBytes};
    record->readPayload(reader, &read);
    REQUIRE(reader.ok());
    REQUIRE(reader.remaining() == 0);
    REQUIRE(read.health == 42);
    REQUIRE(read.speed == 3.5f);
}

TEST_CASE("a component without a visit is registered with no payload adapters", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    const TechEngine::ComponentTypeId id = registry.registerComponent<RegistryPosition>("TechEngine.Position");
    const TechEngine::ComponentTypeRecord* record = registry.find(id);

    REQUIRE(record != nullptr);
    REQUIRE(record->writePayload == nullptr);
    REQUIRE(record->readPayload == nullptr);
    REQUIRE(record->visitReferences == nullptr);
}

TEST_CASE("a component's reference adapter walks the component it is handed", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    const TechEngine::ComponentTypeId id = registry.registerComponent<RegistryLinked>("TechEngine.Linked");
    const TechEngine::ComponentTypeRecord* record = registry.find(id);
    REQUIRE(record != nullptr);
    REQUIRE(record->visitReferences != nullptr);

    RegistryLinked linked;
    linked.health = 3;
    linked.target = TechEngine::Entity{6, 2};
    RegistryEntityRecorder visitor;
    record->visitReferences(visitor, &linked);

    const std::vector<TechEngine::Entity> expected{TechEngine::Entity{6, 2}};
    REQUIRE(visitor.entities == expected);
    REQUIRE(linked.health == 3);
}

TEST_CASE("a component with a visit is registered with both field walks, and one without has neither", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    const TechEngine::ComponentTypeRecord* visited = registry.find(registry.registerComponent<RegistryVisited>("TechEngine.Visited"));
    const TechEngine::ComponentTypeRecord* plain = registry.find(registry.registerComponent<RegistryPosition>("TechEngine.Position"));
    REQUIRE(visited != nullptr);
    REQUIRE(plain != nullptr);

    CHECK(visited->visitFields != nullptr);
    CHECK(visited->editFields != nullptr);
    CHECK(plain->visitFields == nullptr);
    CHECK(plain->editFields == nullptr);
}

TEST_CASE("a component's field walk visits the component it is handed, in place and in visit order", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    const TechEngine::ComponentTypeRecord* record = registry.find(registry.registerComponent<RegistryAssignCounted>("TechEngine.AssignCounted"));
    REQUIRE(record != nullptr);
    REQUIRE(record->visitFields != nullptr);

    RegistryAssignCounted component;
    component.health = 7;
    TechEngineTests::FieldRecorder recorder;
    recorder.writeName = "health";
    recorder.writeValue = 9.0;
    record->visitFields(recorder, &component);

    const std::vector<TechEngineTests::RecordedField> expected{{"health", "uint32"}};
    CHECK(recorder.fields == expected);
    CHECK(component.health == 9);
    CHECK(component.assignments == 0);
}

TEST_CASE("a component's edit walk writes through a copy and assigns it back once", "[core][scene]") {
    TechEngine::ComponentRegistry registry;
    const TechEngine::ComponentTypeRecord* record = registry.find(registry.registerComponent<RegistryAssignCounted>("TechEngine.AssignCounted"));
    REQUIRE(record != nullptr);
    REQUIRE(record->editFields != nullptr);

    RegistryAssignCounted component;
    component.health = 7;
    TechEngineTests::FieldRecorder recorder;
    recorder.writeName = "health";
    recorder.writeValue = 9.0;
    record->editFields(recorder, &component);

    const std::vector<TechEngineTests::RecordedField> expected{{"health", "uint32"}};
    CHECK(recorder.fields == expected);
    CHECK(component.health == 9);
    CHECK(component.assignments == 1);
}
