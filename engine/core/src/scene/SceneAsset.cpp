#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/ComponentStorage.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <scene/SceneAssetEntity.hpp>
#include <scene/SceneAssetHandleFiller.hpp>
#include <scene/SceneAssetIdCollector.hpp>
#include <scene/SceneAssetRowReader.hpp>
#include <scene/SceneAssetRowWriter.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace TechEngine {
    static constexpr std::uint32_t NO_PARENT = 0xFFFFFFFF;

    void SceneAsset::write(const Scene& scene, std::vector<std::byte>& out) const {
        TE_PROFILER_FUNCTION();
        std::vector<Entity> entities = scene.getRoots();
        std::vector<std::uint32_t> parents(entities.size(), NO_PARENT);

        std::unordered_map<Entity, std::uint32_t> rows;
        SceneAssetRowWriter rowWriter{rows, id};

        for (std::size_t index = 0; index < entities.size(); index++) {
            const std::vector<Entity> children = scene.getChildren(entities[index]);
            for (const Entity child: children) {
                entities.push_back(child);
                parents.push_back(static_cast<std::uint32_t>(index));
            }

            rows[entities[index]] = static_cast<std::uint32_t>(index);
        }
        TE_CHECK(entities.size() < NO_PARENT, "A scene asset holds at most {0} entities, the Scene has {1}", NO_PARENT - 1, entities.size());

        Writer writer{out};
        writer.writeHeader(SCENE_KIND);
        writer.write(id.value());
        writer.write(static_cast<std::uint32_t>(entities.size()));

        std::vector<std::byte> blocks;
        std::vector<std::byte> payload;
        std::vector<std::unique_ptr<IComponentStorage>> scratchValues(scene.m_registry->typeCount());
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

                const void* written = component;
                if (record.visitReferences != nullptr) {
                    std::unique_ptr<IComponentStorage>& scratch = scratchValues[record.denseId.value()];
                    if (scratch == nullptr) {
                        scratch = record.createStorage();
                        scratch->appendDefault();
                    }
                    scratch->setCopyFromRaw(0, component);
                    record.visitReferences(rowWriter, scratch->element(0));
                    written = scratch->element(0);
                }

                payload.clear();
                Writer payloadWriter{payload};
                record.writePayload(payloadWriter, written);
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

    std::error_code SceneAsset::read(const std::span<const std::byte> bytes, Scene& scene) {
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
                TE_CHECK(linked, "Validated parent link was rejected while reading a scene asset");
                parent.childCount++;
            }

            for (std::uint32_t block = 0; block < blockCount; block++) {
                StringId tag;
                reader.read(tag);
                reader.readSpan(payload);
                if (!reader.ok()) {
                    return fail(reader.error());
                }

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

        SceneAssetRowReader rowReader{entities};
        for (const SceneAssetEntity& loaded: entities) {
            for (const ComponentTypeRecord& record: registry.records()) {
                if (record.visitReferences == nullptr) {
                    continue;
                }
                void* component = scene.componentRaw(loaded.entity, record.id);
                if (component == nullptr) {
                    continue;
                }
                record.visitReferences(rowReader, component);
            }
        }
        if (rowReader.isMalformed()) {
            return fail(ReadError::Malformed);
        }

        scene.propagateTransforms();
        id = ResourceId{idValue};
        return {};
    }

    std::error_code SceneAsset::load(ResourceCache& cache, Scene& scene, ResourceHandle<SceneBytes>& out) const {
        // A released scene still waiting for a drain has already given up its bytes, so it must
        // be freed before the request, or the request revives it with nothing to stream.
        cache.drainFrees();

        ResourceHandle<SceneBytes> handle;
        const std::error_code requestError = cache.request(id, handle);
        if (requestError) {
            return requestError;
        }

        std::uint32_t acquires = 0;
        const bool counted = cache.getAcquireCount(handle, acquires);
        if (counted && acquires > 1) {
            cache.release(handle);
            TE_LOGGER_WARN("Scene {0:#018x} is already loaded and held, so it is not loaded again", id.value());
            return ReadError::AlreadyLoaded;
        }

        const std::error_code phaseError = cache.runLoadingPhase();
        if (phaseError) {
            cache.release(handle);
            return phaseError;
        }

        ResourceState state = ResourceState::Pending;
        const bool found = cache.getState(handle, state);
        if (!found || state != ResourceState::Ready) {
            cache.release(handle);
            return ReadError::Malformed;
        }

        SceneBytes bytes;
        const bool taken = cache.takePayload(handle, bytes);
        if (!taken) {
            cache.release(handle);
            return ReadError::Malformed;
        }

        SceneAsset parsed;
        const std::error_code readError = parsed.read(bytes.bytes, scene);
        if (readError) {
            cache.release(handle);
            return readError;
        }
        const std::error_code acquireError = acquireReferences(cache, scene, handle);
        if (acquireError) {
            scene.clear();
            cache.release(handle);
            return acquireError;
        }
        out = handle;
        return {};
    }

    std::error_code SceneAsset::acquireReferences(ResourceCache& cache, Scene& scene, const ResourceHandle<SceneBytes> handle) const {
        std::vector<ResourceId> references;
        collectReferences(scene, references);

        std::unordered_map<ResourceId, ResourceSlotId> slots;
        for (const ResourceId reference: references) {
            ResourceSlotId slot;
            const std::error_code dependencyError = cache.addDependency(handle, reference, slot);
            if (dependencyError) {
                return dependencyError;
            }
            slots.emplace(reference, slot);
        }

        const std::error_code phaseError = cache.runLoadingPhase();
        if (phaseError) {
            return phaseError;
        }

        SceneAssetHandleFiller filler{slots};
        visitReferences(scene, filler);
        return {};
    }

    void SceneAsset::collectReferences(Scene& scene, std::vector<ResourceId>& out) const {
        SceneAssetIdCollector collector;
        visitReferences(scene, collector);
        out = collector.getIds();
    }

    void SceneAsset::visitReferences(Scene& scene, ReferenceVisitor& visitor) const {
        std::vector<Entity> entities = scene.getRoots();
        for (std::size_t index = 0; index < entities.size(); index++) {
            const std::vector<Entity> children = scene.getChildren(entities[index]);
            entities.insert(entities.end(), children.begin(), children.end());

            for (const ComponentTypeRecord& record: scene.m_registry->records()) {
                if (record.visitReferences == nullptr) {
                    continue;
                }
                void* component = scene.componentRaw(entities[index], record.id);
                if (component == nullptr) {
                    continue;
                }
                record.visitReferences(visitor, component);
            }
        }
    }

    std::error_code SceneBytes::loadPayload(const std::span<const std::byte> bytes, void* payload, std::vector<ResourceId>& dependencies) {
        SceneBytes* sceneBytes = static_cast<SceneBytes*>(payload);
        sceneBytes->bytes.assign(bytes.begin(), bytes.end());
        dependencies.clear();
        return {};
    }
}
