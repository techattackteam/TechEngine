#pragma once

#include <TechEngine/base/math/Math.hpp>

#include <render/device/draw/IndexedIndirectCommand.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace TechEngine {
    enum class MaterialAlphaMode : std::uint8_t;

    // The enumerator order is the draw order: build() sorts by it, so blended stays last.
    enum class DrawBucket : std::uint8_t {
        Opaque,
        OpaqueMirrored,
        OpaqueDoubleSided,
        Cutout,
        CutoutMirrored,
        CutoutDoubleSided,
        Blended,
        BlendedMirrored,
        BlendedDoubleSided,
    };

    inline constexpr std::size_t DRAW_BUCKET_COUNT = 9;

    struct DrawItem {
        std::uint32_t block = 0;
        DrawBucket bucket = DrawBucket::Opaque;
        // Absolute in the block's index buffer.
        std::uint32_t firstIndex = 0;
        std::uint32_t indexCount = 0;
        std::int32_t baseVertex = 0;
        std::uint32_t materialRecord = 0;
        // Into FrameView::instanceWorlds.
        std::uint32_t instance = 0;
    };

    struct DrawInstance {
        std::uint32_t instance = 0;
        std::uint32_t materialRecord = 0;

        bool operator==(const DrawInstance&) const = default;
    };

    static_assert(sizeof(DrawInstance) == 8, "a draw instance is one uvec2 in the shader's std430 buffer");

    struct DrawBatch {
        std::uint32_t block = 0;
        DrawBucket bucket = DrawBucket::Opaque;
        std::uint32_t firstCommand = 0;
        std::uint32_t commandCount = 0;

        bool operator==(const DrawBatch&) const = default;
    };

    class DrawList {
    private:
        std::vector<DrawItem> m_items;
        std::vector<IndexedIndirectCommand> m_commands;
        std::vector<DrawInstance> m_instances;
        std::vector<DrawBatch> m_batches;

    public:
        void clear();

        void add(const DrawItem& item);

        // Items that share a block, a bucket, an index range and a material become one command. The material is
        // in the key because a bindless handle must be the same for every instance of one command, or GL leaves
        // the sample undefined.
        void build();

        const std::vector<IndexedIndirectCommand>& getCommands() const;

        const std::vector<DrawInstance>& getInstances() const;

        // One per bucket and block that has commands, bucket first, so a second block's opaque draws never follow the first block's blended ones.
        const std::vector<DrawBatch>& getBatches() const;

        static DrawBucket selectBucket(MaterialAlphaMode alphaMode, bool doubleSided, bool mirrored);

        static bool isMirrored(const Mat4& world);
    };
}
