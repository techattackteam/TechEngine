#pragma once

#include <TechEngine/base/math/Math.hpp>

#include <render/device/RenderHandle.hpp>

#include <cstddef>
#include <cstdint>
#include <system_error>
#include <vector>

namespace TechEngine {
    struct Mesh;
    class RenderDevice;

    struct GeometryCapacity {
        std::uint32_t vertices = 0;
        std::uint32_t indices = 0;
    };

    inline constexpr GeometryCapacity GEOMETRY_BLOCK_CAPACITY{1U << 20U, 1U << 22U};

    struct GeometryAttributes {
        Vec3 normal;
        Vec4 tangent;
        Vec2 uv0;
    };

    static_assert(sizeof(Vec3) == 12, "the position stream is 12 bytes per vertex");
    static_assert(sizeof(GeometryAttributes) == 36, "the attribute stream is 36 bytes per vertex");

    struct GeometryRange {
        std::uint32_t block = 0;
        std::uint32_t baseVertex = 0;
        std::uint32_t vertexCount = 0;
        std::uint32_t firstIndex = 0;
        std::uint32_t indexCount = 0;

        bool operator==(const GeometryRange&) const = default;
    };

    struct GeometryFreeRange {
        std::uint32_t offset = 0;
        std::uint32_t count = 0;

        bool operator==(const GeometryFreeRange&) const = default;
    };

    struct GeometryBlock {
        GeometryCapacity capacity;
        // Sorted by offset, so a freed range can merge with the ranges on either side.
        std::vector<GeometryFreeRange> freeVertices;
        std::vector<GeometryFreeRange> freeIndices;
        BufferHandle positionsBuffer;
        BufferHandle attributesBuffer;
        BufferHandle indexBuffer;
    };

    class GeometryBuffers {
    private:
        RenderDevice* m_device = nullptr;
        GeometryCapacity m_blockCapacity;
        std::vector<GeometryBlock> m_blocks;
        std::vector<GeometryAttributes> m_attributeScratch;

    public:
        explicit GeometryBuffers(GeometryCapacity blockCapacity = GEOMETRY_BLOCK_CAPACITY);

        GeometryBuffers(const GeometryBuffers&) = delete;

        GeometryBuffers& operator=(const GeometryBuffers&) = delete;

        GeometryBuffers(GeometryBuffers&&) = delete;

        GeometryBuffers& operator=(GeometryBuffers&&) = delete;

        std::error_code initialize(RenderDevice& device);

        std::error_code place(const Mesh& mesh, GeometryRange& out);

        GeometryRange allocate(std::uint32_t vertexCount, std::uint32_t indexCount);

        void free(const GeometryRange& range);

        std::size_t getBlockCount() const;

        const GeometryBlock& getBlock(std::uint32_t block) const;

        void shutdown();

        static void interleaveAttributes(const Mesh& mesh, std::vector<GeometryAttributes>& out);

    private:
        std::uint32_t addBlock(GeometryCapacity capacity);

        std::error_code createBlockBuffers(GeometryBlock& block);

        void destroyBlockBuffers(GeometryBlock& block);

        bool findBestRange(const std::vector<GeometryFreeRange>& freeRanges, std::uint32_t count, std::size_t& rangeIndex) const;

        std::uint32_t takeFromRange(std::vector<GeometryFreeRange>& freeRanges, std::size_t rangeIndex, std::uint32_t count);

        void returnRange(std::vector<GeometryFreeRange>& freeRanges, std::uint32_t offset, std::uint32_t count);
    };
}
