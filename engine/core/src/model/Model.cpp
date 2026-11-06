#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    static constexpr std::uint64_t MODEL_NODE_MIN_SIZE = sizeof(std::uint32_t) + sizeof(std::uint32_t) + 10 * sizeof(float) + sizeof(std::uint64_t);

    void Model::write(std::vector<std::byte>& out) const {
        TE_PROFILER_FUNCTION();
        Writer writer{out};
        writer.writeHeader(MODEL_KIND);
        writer.write(id.value());
        importBlock.write(writer);
        writer.write(static_cast<std::uint32_t>(nodes.size()));
        for (const ModelNode& node: nodes) {
            writer.write(node.name);
            writer.write(node.parent);

            writer.write(node.local.position.x);
            writer.write(node.local.position.y);
            writer.write(node.local.position.z);

            writer.write(node.local.rotation.w);
            writer.write(node.local.rotation.x);
            writer.write(node.local.rotation.y);
            writer.write(node.local.rotation.z);

            writer.write(node.local.scale.x);
            writer.write(node.local.scale.y);
            writer.write(node.local.scale.z);

            writer.write(node.mesh.value());
        }
    }

    std::error_code Model::read(const std::span<const std::byte> bytes) {
        TE_PROFILER_FUNCTION();
        Reader reader{bytes};
        BlobHeader header;
        reader.readHeader(MODEL_KIND, header);
        std::uint64_t idValue = 0;
        reader.read(idValue);
        if (!reader.ok()) {
            return reader.error();
        }

        ImportBlock readImportBlock;
        const std::error_code importError = readImportBlock.read(reader);
        if (importError) {
            return importError;
        }

        std::uint32_t nodeCount = 0;
        reader.read(nodeCount);
        if (!reader.ok()) {
            return reader.error();
        }
        if (static_cast<std::uint64_t>(nodeCount) * MODEL_NODE_MIN_SIZE > reader.remaining()) {
            return ReadError::Truncated;
        }

        std::vector<ModelNode> readNodes(nodeCount);
        for (ModelNode& node: readNodes) {
            reader.read(node.name);
            reader.read(node.parent);

            reader.read(node.local.position.x);
            reader.read(node.local.position.y);
            reader.read(node.local.position.z);

            reader.read(node.local.rotation.w);
            reader.read(node.local.rotation.x);
            reader.read(node.local.rotation.y);
            reader.read(node.local.rotation.z);

            reader.read(node.local.scale.x);
            reader.read(node.local.scale.y);
            reader.read(node.local.scale.z);

            std::uint64_t meshValue = 0;
            reader.read(meshValue);
            node.mesh = ResourceId{meshValue};
        }
        if (!reader.ok()) {
            return reader.error();
        }
        if (reader.remaining() != 0) {
            return ReadError::Malformed;
        }

        if (readNodes.empty() || readNodes[0].parent != MODEL_NO_PARENT) {
            return ReadError::Malformed;
        }
        for (std::uint32_t i = 1; i < nodeCount; i++) {
            if (readNodes[i].parent >= i) {
                return ReadError::Malformed;
            }
        }

        id = ResourceId{idValue};
        importBlock = std::move(readImportBlock);
        nodes = std::move(readNodes);
        return {};
    }
}
