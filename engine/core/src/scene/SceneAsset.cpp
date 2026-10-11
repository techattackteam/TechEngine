#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/ComponentStorage.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <system_error>
#include <vector>

namespace TechEngine {
    static constexpr std::uint32_t NO_PARENT = 0xFFFFFFFF;

    struct SceneAssetEntity {
        Entity entity;
        std::size_t childCount = 0;
    };

    void writeSceneAsset(const Scene& scene, const ResourceId id, std::vector<std::byte>& out) {
        TE_PROFILER_FUNCTION();
        std::vector<Entity> entities = scene.getRoots();
        std::vector<std::uint32_t> parents(entities.size(), NO_PARENT);
        for (std::size_t index = 0; index < entities.size(); index++) {
            const std::vector<Entity> children = scene.getChildren(entities[index]);
            for (const Entity child: children) {
                entities.push_back(child);
                parents.push_back(static_cast<std::uint32_t>(index));
            }
        }
        TE_CHECK(entities.size() < NO_PARENT, "A scene asset holds at most {0} entities, the Scene has {1}", NO_PARENT - 1, entities.size());

        Writer writer{out};
        writer.writeHeader(SCENE_KIND);
        writer.write(id.value());
        writer.write(static_cast<std::uint32_t>(entities.size()));

        std::vector<std::byte> blocks;
        std::vector<std::byte> payload;
        for (std::size_t index = 0; index < entities.size(); index++) {
            blocks.clear();
            Writer blockWriter{blocks};
            std::uint32_t blockCount = 0;
            for (const ComponentTypeRecord& record: scene.m_registry->records()) {
                if (record.writePayload == nullptr) {
                    continue;
                }
                const void* component = scene.componentRaw(entities[index], record.id);
                if (component == nullptr) {
                    continue;
                }

                payload.clear();
                Writer payloadWriter{payload};
                record.writePayload(payloadWriter, component);
                TE_CHECK(payload.size() <= BLOB_MAX_COUNT, "A {0} payload of {1} bytes exceeds the u32 block length", record.tag, payload.size());

                blockWriter.write(record.id.stringId());
                blockWriter.write(static_cast<std::uint32_t>(payload.size()));
                blockWriter.writeBytes(payload);
                blockCount++;
            }

            writer.write(static_cast<std::uint32_t>(index));
            writer.write(parents[index]);
            writer.write(blockCount);
            writer.writeBytes(blocks);
        }
    }

    std::error_code readSceneAsset(const std::span<const std::byte> bytes, Scene& scene, ResourceId& id) {
        TE_PROFILER_FUNCTION();
        TE_CHECK(scene.getRoots().empty(), "A scene asset is read into an empty Scene only");
        const auto fail = [&scene](const std::error_code error) {
            scene.clear();
            return error;
        };

        Reader reader{bytes};
        BlobHeader header;
        reader.readHeader(SCENE_KIND, header);
        std::uint64_t idValue = 0;
        reader.read(idValue);
        std::uint32_t entityCount = 0;
        reader.read(entityCount);
        if (!reader.ok()) {
            return fail(reader.error());
        }

        const ComponentRegistry& registry = *scene.m_registry;
        std::vector<SceneAssetEntity> entities;
        std::vector<std::unique_ptr<IComponentStorage>> scratchValues(registry.typeCount());
        std::vector<std::byte> payload;

        for (std::uint32_t row = 0; row < entityCount; row++) {
            std::uint32_t authoringId = 0;
            std::uint32_t parentId = 0;
            std::uint32_t blockCount = 0;
            reader.read(authoringId);
            reader.read(parentId);
            reader.read(blockCount);
            if (!reader.ok()) {
                return fail(reader.error());
            }
            if (authoringId != row || (parentId != NO_PARENT && parentId >= row)) {
                return fail(ReadError::Malformed);
            }

            const Entity entity = scene.createEntity();
            entities.push_back(SceneAssetEntity{entity});
            if (parentId != NO_PARENT) {
                SceneAssetEntity& parent = entities[parentId];
                const bool linked = scene.setParent(entity, parent.entity, parent.childCount);
                if (!linked) {
                    return fail(ReadError::Malformed);
                }
                parent.childCount++;
            }

            for (std::uint32_t block = 0; block < blockCount; block++) {
                StringId tag;
                std::uint32_t length = 0;
                reader.read(tag);
                reader.read(length);
                if (!reader.ok()) {
                    return fail(reader.error());
                }
                if (reader.remaining() < length) {
                    return fail(ReadError::Truncated);
                }
                payload.resize(length);
                reader.readBytes(payload);

                const ComponentTypeRecord* record = registry.find(ComponentTypeId{tag});
                if (record == nullptr || record->readPayload == nullptr) {
                    TE_LOGGER_WARN("Scene asset {0:#018x}: skipped a block with the unregistered tag {1}", idValue, tag);
                    continue;
                }
                std::unique_ptr<IComponentStorage>& scratch = scratchValues[record->denseId.value()];
                if (scratch == nullptr) {
                    scratch = record->createStorage();
                } else {
                    scratch->popBack();
                }
                scratch->appendDefault();

                Reader payloadReader{payload};
                record->readPayload(payloadReader, scratch->element(0));
                if (!payloadReader.ok() || payloadReader.remaining() != 0) {
                    TE_LOGGER_WARN("Scene asset {0:#018x}: skipped a {1} block whose payload does not match its length", idValue, record->tag);
                    continue;
                }

                // Read twice on purpose: the scratch pass proves the payload fits before the live
                // component is touched, and an in-place read skips propagation until the end.
                void* existing = scene.componentRaw(entity, record->id);
                if (existing != nullptr) {
                    Reader inPlaceReader{payload};
                    record->readPayload(inPlaceReader, existing);
                } else if (!scene.addComponentInternal(entity, record->id, scratch->element(0))) {
                    return fail(ReadError::Malformed);
                }
            }
        }
        if (reader.remaining() != 0) {
            return fail(ReadError::Malformed);
        }

        scene.propagateTransforms();
        id = ResourceId{idValue};
        return {};
    }
}
