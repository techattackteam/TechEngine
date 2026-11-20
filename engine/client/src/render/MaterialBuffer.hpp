#pragma once

#include <render/device/RenderHandle.hpp>
#include <render/device/material/Material.hpp>

#include <cstdint>
#include <system_error>
#include <vector>

namespace TechEngine {
    class RenderDevice;

    inline constexpr std::uint32_t DEFAULT_MATERIAL_RECORD = 0;

    inline constexpr std::uint32_t MATERIAL_BUFFER_INITIAL_CAPACITY = 1024;

    class MaterialBuffer {
    private:
        RenderDevice* m_device = nullptr;
        BufferHandle m_buffer;
        std::uint32_t m_bufferCapacity = 0;
        std::vector<GpuMaterial> m_records;
        std::vector<std::uint32_t> m_freeRecords;
        std::uint32_t m_nextRecord = DEFAULT_MATERIAL_RECORD + 1;

    public:
        explicit MaterialBuffer(std::uint32_t initialCapacity = MATERIAL_BUFFER_INITIAL_CAPACITY);

        MaterialBuffer(const MaterialBuffer&) = delete;

        MaterialBuffer& operator=(const MaterialBuffer&) = delete;

        MaterialBuffer(MaterialBuffer&&) = delete;

        MaterialBuffer& operator=(MaterialBuffer&&) = delete;

        std::error_code initialize(RenderDevice& device);

        std::uint32_t allocate();

        std::error_code write(std::uint32_t record, const GpuMaterial& material);

        void free(std::uint32_t record);

        const GpuMaterial& getRecord(std::uint32_t record) const;

        std::uint32_t getCapacity() const;

        // Growing replaces the buffer, so a frame binds the handle it gets after its uploads are applied, never one kept from an earlier frame.
        BufferHandle getBuffer() const;

        void shutdown();

        static GpuMaterial getDefaultMaterial();

    private:
        std::error_code growBuffer();
    };
}
