#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/material/Material.hpp>

#include <render/MaterialBuffer.hpp>
#include <render/device/RenderDevice.hpp>

#include <algorithm>

namespace TechEngine {
    MaterialBuffer::MaterialBuffer(const std::uint32_t initialCapacity) : m_records(initialCapacity) {
    }

    std::error_code MaterialBuffer::initialize(RenderDevice& device) {
        m_device = &device;
        m_records[DEFAULT_MATERIAL_RECORD] = getDefaultMaterial();
        return growBuffer();
    }

    std::uint32_t MaterialBuffer::allocate() {
        if (!m_freeRecords.empty()) {
            const std::uint32_t record = m_freeRecords.back();
            m_freeRecords.pop_back();
            return record;
        }
        if (m_nextRecord >= m_records.size()) {
            m_records.resize(m_records.size() * 2);
        }
        return m_nextRecord++;
    }

    std::error_code MaterialBuffer::write(const std::uint32_t record, const GpuMaterial& material) {
        m_records[record] = material;
        if (m_bufferCapacity < m_records.size()) {
            return growBuffer();
        }
        const std::size_t offset = static_cast<std::size_t>(record) * sizeof(GpuMaterial);
        m_device->updateBuffer(m_buffer, offset, std::as_bytes(std::span{&material, 1}));
        return {};
    }

    void MaterialBuffer::free(const std::uint32_t record) {
        const bool handedOut = record != DEFAULT_MATERIAL_RECORD && record < m_nextRecord;
        TE_CHECK(handedOut, "MaterialBuffer::free called with record {0}, which was never handed out", record);
        if (!handedOut) {
            return;
        }
        const bool alreadyFree = std::ranges::find(m_freeRecords, record) != m_freeRecords.end();
        TE_CHECK(!alreadyFree, "MaterialBuffer::free called with record {0}, which is already free", record);
        if (alreadyFree) {
            return;
        }
        m_freeRecords.push_back(record);
    }

    const GpuMaterial& MaterialBuffer::getRecord(const std::uint32_t record) const {
        TE_CHECK(record < m_records.size(), "MaterialBuffer::getRecord called with record {0} of {1}", record, m_records.size());
        return m_records[record];
    }

    std::uint32_t MaterialBuffer::getCapacity() const {
        return static_cast<std::uint32_t>(m_records.size());
    }

    BufferHandle MaterialBuffer::getBuffer() const {
        return m_buffer;
    }

    void MaterialBuffer::shutdown() {
        if (m_buffer.valid()) {
            m_device->destroyBuffer(m_buffer);
            m_buffer = {};
        }
        m_bufferCapacity = 0;
    }

    GpuMaterial MaterialBuffer::getDefaultMaterial() {
        GpuMaterial material;
        material.albedo = Vec4{1.0F, 1.0F, 1.0F, 1.0F};
        material.occlusionStrength = 1.0F;
        material.roughnessFactor = 1.0F;
        material.metallicFactor = 0.0F;
        material.normalScale = 1.0F;
        material.emissiveFactor = Vec3{0.0F, 0.0F, 0.0F};
        material.shadingModel = static_cast<std::uint32_t>(MaterialShadingModel::Lit);
        return material;
    }

    std::error_code MaterialBuffer::growBuffer() {
        BufferDescription description;
        description.size = m_records.size() * sizeof(GpuMaterial);
        description.usage = BufferUsage::Dynamic;
        description.initialData = std::as_bytes(std::span{m_records});
        BufferHandle buffer;
        const std::error_code createError = m_device->createBuffer(description, buffer);
        if (createError) {
            return createError;
        }
        // GL keeps a deleted buffer alive until the frames that bound it have finished.
        if (m_buffer.valid()) {
            m_device->destroyBuffer(m_buffer);
        }
        m_buffer = buffer;
        m_bufferCapacity = static_cast<std::uint32_t>(m_records.size());
        return {};
    }
}
