#include <TechEngine/core/material/Material.hpp>

#include <render/DrawList.hpp>

#include <algorithm>
#include <cstddef>
#include <tuple>

namespace TechEngine {
    void DrawList::clear() {
        m_items.clear();
        m_commands.clear();
        m_instances.clear();
        m_batches.clear();
    }

    void DrawList::add(const DrawItem& item) {
        m_items.push_back(item);
    }

    void DrawList::build() {
        m_commands.clear();
        m_instances.clear();
        m_batches.clear();
        std::ranges::sort(m_items, [](const DrawItem& a, const DrawItem& b) {
            return std::tie(a.bucket, a.block, a.firstIndex, a.indexCount, a.baseVertex, a.materialRecord) < std::tie(b.bucket, b.block, b.firstIndex, b.indexCount, b.baseVertex, b.materialRecord);
        });

        for (std::size_t i = 0; i < m_items.size(); i++) {
            const DrawItem& item = m_items[i];
            const bool startsBatch = i == 0 || item.bucket != m_items[i - 1].bucket || item.block != m_items[i - 1].block;
            const bool startsCommand = startsBatch || item.firstIndex != m_items[i - 1].firstIndex || item.indexCount != m_items[i - 1].indexCount || item.baseVertex != m_items[i - 1].baseVertex || item.materialRecord != m_items[i - 1].materialRecord;
            if (startsBatch) {
                m_batches.push_back(DrawBatch{item.block, item.bucket, static_cast<std::uint32_t>(m_commands.size()), 0});
            }
            if (startsCommand) {
                m_commands.push_back(IndexedIndirectCommand{item.indexCount, 0, item.firstIndex, item.baseVertex, static_cast<std::uint32_t>(m_instances.size())});
                m_batches.back().commandCount++;
            }
            m_commands.back().instanceCount++;
            m_instances.push_back(DrawInstance{item.instance, item.materialRecord});
        }
    }

    const std::vector<IndexedIndirectCommand>& DrawList::getCommands() const {
        return m_commands;
    }

    const std::vector<DrawInstance>& DrawList::getInstances() const {
        return m_instances;
    }

    const std::vector<DrawBatch>& DrawList::getBatches() const {
        return m_batches;
    }

    DrawBucket DrawList::selectBucket(const MaterialAlphaMode alphaMode, const bool doubleSided, const bool mirrored) {
        if (doubleSided) {
            switch (alphaMode) {
                case MaterialAlphaMode::Opaque:
                    return DrawBucket::OpaqueDoubleSided;
                case MaterialAlphaMode::Cutout:
                    return DrawBucket::CutoutDoubleSided;
                case MaterialAlphaMode::Blended:
                    return DrawBucket::BlendedDoubleSided;
            }
        }

        switch (alphaMode) {
            case MaterialAlphaMode::Opaque:
                return mirrored ? DrawBucket::OpaqueMirrored : DrawBucket::Opaque;
            case MaterialAlphaMode::Cutout:
                return mirrored ? DrawBucket::CutoutMirrored : DrawBucket::Cutout;
            case MaterialAlphaMode::Blended:
                return mirrored ? DrawBucket::BlendedMirrored : DrawBucket::Blended;
        }
        return DrawBucket::Opaque;
    }

    bool DrawList::isMirrored(const Mat4& world) {
        return world[0][0] * world[1][1] * world[2][2] + world[0][1] * world[1][2] * world[2][0] + world[0][2] * world[1][0] * world[2][1] - world[0][2] * world[1][1] * world[2][0] - world[0][1] * world[1][0] * world[2][2] - world[0][0] * world[1][2] * world[2][1] < 0.0f;
    }
}
