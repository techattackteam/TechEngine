#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

using TechEngine::BlobHeader;
using TechEngine::Model;
using TechEngine::MODEL_KIND;
using TechEngine::MODEL_NO_PARENT;
using TechEngine::ModelNode;
using TechEngine::Quat;
using TechEngine::Reader;
using TechEngine::ReadError;
using TechEngine::ResourceId;
using TechEngine::TransformValues;
using TechEngine::UpAxis;
using TechEngine::Vec3;
using TechEngine::Writer;

static constexpr ResourceId MODEL_ASSET_ID{0x6D6F64656C000001};
static constexpr ResourceId OTHER_MODEL_ID{0x6D6F64656C000002};
static constexpr ResourceId BODY_MESH_ID{0x6D65736800000011};
static constexpr ResourceId WHEEL_MESH_ID{0x6D65736800000012};

static ModelNode makeNode(const std::string& name, const std::uint32_t parent, const TransformValues& local, const ResourceId mesh) {
    ModelNode node;
    node.name = name;
    node.parent = parent;
    node.local = local;
    node.mesh = mesh;
    return node;
}

static TransformValues makeLocal(const Vec3& position, const Quat& rotation, const Vec3& scale) {
    TransformValues local;
    local.position = position;
    local.rotation = rotation;
    local.scale = scale;
    return local;
}

static Model makeKart() {
    Model model;
    model.id = MODEL_ASSET_ID;
    model.importBlock.sourcePath = "assets/common/Kart.gltf";
    model.importBlock.settings.scale = 0.01F;
    model.importBlock.settings.upAxis = UpAxis::Z;
    const Quat halfTurnY = Quat(0.0F, 0.0F, 1.0F, 0.0F);
    const Quat quarterTurnX = Quat(0.70710677F, 0.70710677F, 0.0F, 0.0F);
    model.nodes = {
        makeNode("Kart", MODEL_NO_PARENT, TransformValues{}, ResourceId{}),
        makeNode("Body", 0, makeLocal(Vec3(0.0F, 0.3F, 0.0F), Quat(1.0F, 0.0F, 0.0F, 0.0F), Vec3(1.0F)), BODY_MESH_ID),
        makeNode("Axle", 0, makeLocal(Vec3(0.0F, 0.1F, 0.8F), quarterTurnX, Vec3(1.0F)), ResourceId{}),
        makeNode("Wheel_L", 2, makeLocal(Vec3(-0.6F, 0.0F, 0.0F), Quat(1.0F, 0.0F, 0.0F, 0.0F), Vec3(1.0F, 2.0F, 0.5F)), WHEEL_MESH_ID),
        makeNode("Wheel_R", 2, makeLocal(Vec3(0.6F, 0.0F, 0.0F), halfTurnY, Vec3(1.0F, 2.0F, 0.5F)), WHEEL_MESH_ID),
    };
    return model;
}

static Model makeLoneRoot(const ResourceId id) {
    Model model;
    model.id = id;
    model.nodes = {makeNode("Lone", MODEL_NO_PARENT, makeLocal(Vec3(1.0F, 2.0F, 3.0F), Quat(1.0F, 0.0F, 0.0F, 0.0F), Vec3(2.0F)), BODY_MESH_ID)};
    return model;
}

static std::vector<std::byte> writeModel(const Model& model) {
    std::vector<std::byte> bytes;
    model.write(bytes);
    return bytes;
}

static void checkNodesEqual(const std::vector<ModelNode>& actual, const std::vector<ModelNode>& expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); i++) {
        CHECK(actual[i].name == expected[i].name);
        CHECK(actual[i].parent == expected[i].parent);
        CHECK(actual[i].local.position == expected[i].local.position);
        CHECK(actual[i].local.rotation == expected[i].local.rotation);
        CHECK(actual[i].local.scale == expected[i].local.scale);
        CHECK(actual[i].mesh == expected[i].mesh);
    }
}

static void checkModelsEqual(const Model& actual, const Model& expected) {
    CHECK(actual.id == expected.id);
    CHECK(actual.importBlock == expected.importBlock);
    checkNodesEqual(actual.nodes, expected.nodes);
}

TEST_CASE("a model with a root and nested nodes writes and reads back equal", "[core][model]") {
    const Model written = makeKart();
    Model loaded;

    REQUIRE(loaded.read(writeModel(written)) == std::error_code{});

    checkModelsEqual(loaded, written);
    CHECK_FALSE(loaded.nodes[0].mesh.valid());
    CHECK_FALSE(loaded.nodes[2].mesh.valid());
}

TEST_CASE("a model with only a root round-trips", "[core][model]") {
    const Model written = makeLoneRoot(MODEL_ASSET_ID);
    Model loaded = makeKart();

    REQUIRE(loaded.read(writeModel(written)) == std::error_code{});

    checkModelsEqual(loaded, written);
}

TEST_CASE("the model's import block round-trips inside the model", "[core][model][import]") {
    Model written = makeKart();
    written.importBlock.sourceIndex = 3;
    written.importBlock.sourceName = "KartScene";
    Model loaded;

    REQUIRE(loaded.read(writeModel(written)) == std::error_code{});

    CHECK(loaded.importBlock == written.importBlock);
}

TEST_CASE("a model's id comes straight after its header, where the asset index reads it", "[core][model]") {
    const std::vector<std::byte> bytes = writeModel(makeKart());
    Reader reader{bytes};
    BlobHeader header;
    reader.readHeader(MODEL_KIND, header);
    std::uint64_t id = 0;

    reader.read(id);

    REQUIRE(reader.ok());
    CHECK(id == MODEL_ASSET_ID.value());
}

TEST_CASE("read refuses a node list that is not parent-first with one root, and leaves the model untouched", "[core][model]") {
    Model invalid = makeKart();

    SECTION("a parent after its child") {
        invalid.nodes[1].parent = 4;
    }

    SECTION("a node that is its own parent") {
        invalid.nodes[3].parent = 3;
    }

    SECTION("a parent index past the end of the list") {
        invalid.nodes[4].parent = 99;
    }

    SECTION("two roots") {
        invalid.nodes[2].parent = MODEL_NO_PARENT;
    }

    SECTION("a first node with a parent, so no root") {
        invalid.nodes[0].parent = 1;
    }

    SECTION("no nodes at all") {
        invalid.nodes.clear();
    }

    Model loaded = makeLoneRoot(OTHER_MODEL_ID);
    REQUIRE(loaded.read(writeModel(invalid)) == ReadError::Malformed);
    checkModelsEqual(loaded, makeLoneRoot(OTHER_MODEL_ID));
}

TEST_CASE("read rejects a model whose bytes break the format and leaves the model untouched", "[core][model]") {
    std::vector<std::byte> bytes;
    std::error_code expected;

    SECTION("bytes after the last node") {
        bytes = writeModel(makeKart());
        bytes.push_back(std::byte{0});
        expected = ReadError::Malformed;
    }

    SECTION("bytes cut inside the last node") {
        bytes = writeModel(makeKart());
        bytes.resize(bytes.size() - 1);
        expected = ReadError::Truncated;
    }

    SECTION("an import block the block reader refuses") {
        Model model = makeKart();
        model.importBlock.settings.upAxis = static_cast<UpAxis>(2);
        bytes = writeModel(model);
        expected = ReadError::Malformed;
    }

    SECTION("another asset kind's magic") {
        std::vector<std::byte> scene;
        Writer writer{scene};
        writer.writeHeader(TechEngine::SCENE_KIND);
        writer.write(MODEL_ASSET_ID.value());
        bytes = std::move(scene);
        expected = ReadError::BadMagic;
    }

    SECTION("a newer model version") {
        std::vector<std::byte> newer;
        Writer writer{newer};
        writer.writeHeader(TechEngine::BlobKind{MODEL_KIND.magic, static_cast<std::uint16_t>(MODEL_KIND.version + 1)});
        writer.write(MODEL_ASSET_ID.value());
        bytes = std::move(newer);
        expected = ReadError::BadVersion;
    }

    Model loaded = makeLoneRoot(OTHER_MODEL_ID);
    REQUIRE(loaded.read(bytes) == expected);
    checkModelsEqual(loaded, makeLoneRoot(OTHER_MODEL_ID));
}
