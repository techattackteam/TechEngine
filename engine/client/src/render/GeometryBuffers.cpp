#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>

#include <render/GeometryBuffers.hpp>
#include <render/device/RenderDevice.hpp>

#include <algorithm>
#include <span>
#include <utility>

namespace TechEngine {
    GeometryBuffers::GeometryBuffers(const GeometryCapacity blockCapacity) : m_blockCapacity(blockCapacity) {
    }

    std::error_code GeometryBuffers::initialize(RenderDevice& device) {
        m_device = &device;
        const std::uint32_t block = addBlock(m_blockCapacity);
        const std::error_code error = createBlockBuffers(m_blocks[block]);
        if (error) {
            TE_LOGGER_ERROR("GeometryBuffers: the first block's buffers were not created: {0}", error.message());
        }
        return error;
    }

    std::error_code GeometryBuffers::place(const Mesh& mesh, GeometryRange& out) {
        if (mesh.positions.empty() || mesh.indices.empty()) {
            out = {};
            return {};
        }
        out = allocate(static_cast<std::uint32_t>(mesh.positions.size()), static_cast<std::uint32_t>(mesh.indices.size()));
        GeometryBlock& block = m_blocks[out.block];
        if (!block.positionsBuffer.valid()) {
            const std::error_code blockError = createBlockBuffers(block);
            if (blockError) {
                free(out);
                out = {};
                return blockError;
            }
        }
        interleaveAttributes(mesh, m_attributeScratch);
        m_device->updateBuffer(block.positionsBuffer, static_cast<std::size_t>(out.baseVertex) * 12, std::as_bytes(std::span{mesh.positions}));
        m_device->updateBuffer(block.attributesBuffer, static_cast<std::size_t>(out.baseVertex) * 36, std::as_bytes(std::span{m_attributeScratch}));
        m_device->updateBuffer(block.indexBuffer, static_cast<std::size_t>(out.firstIndex) * 4, std::as_bytes(std::span{mesh.indices}));
        return {};
    }

    GeometryRange GeometryBuffers::allocate(std::uint32_t vertexCount, std::uint32_t indexCount) {
        if (vertexCount == 0 || indexCount == 0) {
            return {};
        }
        std::size_t blockIndex = 0;
        std::size_t vertexRangeIndex = 0;
        std::size_t indexRangeIndex = 0;
        bool found = false;
        for (; blockIndex < m_blocks.size(); blockIndex++) {
            const bool vertexFound = findBestRange(m_blocks[blockIndex].freeVertices, vertexCount, vertexRangeIndex);
            const bool indexFound = findBestRange(m_blocks[blockIndex].freeIndices, indexCount, indexRangeIndex);
            if (vertexFound && indexFound) {
                found = true;
                break;
            }
        }
        if (!found) {
            const bool oversized = vertexCount > m_blockCapacity.vertices || indexCount > m_blockCapacity.indices;
            const GeometryCapacity capacity = oversized ? GeometryCapacity{vertexCount, indexCount} : m_blockCapacity;
            blockIndex = addBlock(capacity);
            findBestRange(m_blocks[blockIndex].freeVertices, vertexCount, vertexRangeIndex);
            findBestRange(m_blocks[blockIndex].freeIndices, indexCount, indexRangeIndex);
        }

        GeometryRange range;
        range.block = static_cast<std::uint32_t>(blockIndex);
        range.vertexCount = vertexCount;
        range.indexCount = indexCount;
        range.baseVertex = takeFromRange(m_blocks[blockIndex].freeVertices, vertexRangeIndex, vertexCount);
        range.firstIndex = takeFromRange(m_blocks[blockIndex].freeIndices, indexRangeIndex, indexCount);
        return range;
    }

    void GeometryBuffers::free(const GeometryRange& range) {
        if (range.vertexCount == 0 && range.indexCount == 0) {
            return;
        }
        const bool known = range.block < m_blocks.size();
        TE_CHECK(known, "GeometryBuffers::free called with block {0} of {1}", range.block, m_blocks.size());
        if (!known) {
            return;
        }
        GeometryBlock& block = m_blocks[range.block];
        returnRange(block.freeVertices, range.baseVertex, range.vertexCount);
        returnRange(block.freeIndices, range.firstIndex, range.indexCount);
    }

    std::size_t GeometryBuffers::getBlockCount() const {
        return m_blocks.size();
    }

    const GeometryBlock& GeometryBuffers::getBlock(const std::uint32_t block) const {
        TE_CHECK(block < m_blocks.size(), "GeometryBuffers::getBlock called with block {0} of {1}", block, m_blocks.size());
        return m_blocks[block];
    }

    void GeometryBuffers::shutdown() {
        for (GeometryBlock& block: m_blocks) {
            destroyBlockBuffers(block);
        }
    }

    void GeometryBuffers::interleaveAttributes(const Mesh& mesh, std::vector<GeometryAttributes>& out) {
        out.resize(mesh.positions.size());
        for (std::size_t i = 0; i < mesh.positions.size(); i++) {
            GeometryAttributes& attributes = out[i];
            if (i < mesh.normals.size()) {
                attributes.normal = mesh.normals[i];
            } else {
                attributes.normal = {0.0f, 1.0f, 0.0f};
            }
            if (i < mesh.tangents.size()) {
                attributes.tangent = mesh.tangents[i];
                // Flipping V below reverses the bitangent, and w is its sign.
                attributes.tangent.w = -attributes.tangent.w;
            } else {
                attributes.tangent = {1.0f, 0.0f, 0.0f, 1.0f};
            }
            if (i < mesh.uv0.size()) {
                attributes.uv0 = mesh.uv0[i];
                // Texture rows are stored top row first, and the mesh's V is 0 at the bottom of the picture.
                attributes.uv0.y = 1.0f - attributes.uv0.y;
            } else {
                attributes.uv0 = {0.0f, 0.0f};
            }
        }
    }

    std::uint32_t GeometryBuffers::addBlock(GeometryCapacity capacity) {
        GeometryBlock block;
        block.capacity = capacity;
        block.freeVertices.push_back({0, capacity.vertices});
        block.freeIndices.push_back({0, capacity.indices});
        m_blocks.push_back(std::move(block));
        return static_cast<std::uint32_t>(m_blocks.size() - 1);
    }

    std::error_code GeometryBuffers::createBlockBuffers(GeometryBlock& block) {
        const std::error_code positionError = m_device->createBuffer(BufferDescription{static_cast<std::size_t>(block.capacity.vertices) * 12, BufferUsage::Dynamic, {}}, block.positionsBuffer);
        if (positionError) {
            return positionError;
        }
        const std::error_code attributesError = m_device->createBuffer(BufferDescription{static_cast<std::size_t>(block.capacity.vertices) * 36, BufferUsage::Dynamic, {}}, block.attributesBuffer);
        if (attributesError) {
            destroyBlockBuffers(block);
            return attributesError;
        }
        const std::error_code indicesError = m_device->createBuffer(BufferDescription{static_cast<std::size_t>(block.capacity.indices) * 4, BufferUsage::Dynamic, {}}, block.indexBuffer);
        if (indicesError) {
            destroyBlockBuffers(block);
            return indicesError;
        }

        return {};
    }

    void GeometryBuffers::destroyBlockBuffers(GeometryBlock& block) {
        if (block.positionsBuffer.valid()) {
            m_device->destroyBuffer(block.positionsBuffer);
            block.positionsBuffer = {};
        }
        if (block.attributesBuffer.valid()) {
            m_device->destroyBuffer(block.attributesBuffer);
            block.attributesBuffer = {};
        }
        if (block.indexBuffer.valid()) {
            m_device->destroyBuffer(block.indexBuffer);
            block.indexBuffer = {};
        }
    }

    bool GeometryBuffers::findBestRange(const std::vector<GeometryFreeRange>& freeRanges, std::uint32_t count, std::size_t& rangeIndex) const {
        bool found = false;
        for (std::size_t i = 0; i < freeRanges.size(); i++) {
            if (freeRanges[i].count >= count) {
                if (!found || freeRanges[i].count < freeRanges[rangeIndex].count) {
                    rangeIndex = i;
                    found = true;
                }
            }
        }
        return found;
    }

    std::uint32_t GeometryBuffers::takeFromRange(std::vector<GeometryFreeRange>& freeRanges, std::size_t rangeIndex, std::uint32_t count) {
        const bool fits = rangeIndex < freeRanges.size() && freeRanges[rangeIndex].count >= count;
        TE_CHECK(fits, "GeometryBuffers::takeFromRange called with free range {0} of {1}, which cannot hold {2}", rangeIndex, freeRanges.size(), count);
        if (!fits) {
            return 0;
        }
        GeometryFreeRange& range = freeRanges[rangeIndex];
        const std::uint32_t offset = range.offset;
        range.offset += count;
        range.count -= count;
        if (range.count == 0) {
            freeRanges.erase(freeRanges.begin() + rangeIndex);
        }
        return offset;
    }

    void GeometryBuffers::returnRange(std::vector<GeometryFreeRange>& freeRanges, std::uint32_t offset, std::uint32_t count) {
        const auto following = std::lower_bound(freeRanges.begin(), freeRanges.end(), offset, [](const GeometryFreeRange& range, const std::uint32_t value) {
            return range.offset < value;
        });
        const bool clearOfFollowing = following == freeRanges.end() || offset + count <= following->offset;
        const bool clearOfPrevious = following == freeRanges.begin() || (following - 1)->offset + (following - 1)->count <= offset;
        TE_CHECK(clearOfFollowing && clearOfPrevious, "GeometryBuffers::returnRange called with {0} units at {1}, which overlap a free range", count, offset);
        if (!clearOfFollowing || !clearOfPrevious) {
            return;
        }
        const bool touchesPrevious = following != freeRanges.begin() && (following - 1)->offset + (following - 1)->count == offset;
        const bool touchesFollowing = following != freeRanges.end() && offset + count == following->offset;
        if (touchesPrevious && touchesFollowing) {
            (following - 1)->count += count + following->count;
            freeRanges.erase(following);
        } else if (touchesPrevious) {
            (following - 1)->count += count;
        } else if (touchesFollowing) {
            following->offset = offset;
            following->count += count;
        } else {
            freeRanges.insert(following, GeometryFreeRange{offset, count});
        }
    }
}
