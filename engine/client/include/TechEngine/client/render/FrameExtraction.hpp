#pragma once

namespace TechEngine {
    class Scene;
    struct FramePacket;

    class FrameExtraction {
    public:
        // Writes the scene's half of the packet: its instances, their materials and its camera. The
        // timing fields, the sequence and the input stay the caller's.
        static void extractFramePacket(const Scene& scene, bool& warnedNoCamera, FramePacket& out);
    };
}
