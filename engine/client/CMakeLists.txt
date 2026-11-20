techengine_module(client
        SOURCES src/Client.cpp
        src/ClientError.cpp
        src/render/DrawList.cpp
        src/render/FrameExtraction.cpp
        src/render/FramePacketPool.cpp
        src/render/GeometryBuffers.cpp
        src/render/GpuCopies.cpp
        src/render/MaterialBuffer.cpp
        src/render/PacketHistory.cpp
        src/render/PacketMailbox.cpp
        src/render/FrameRenderer.cpp
        src/render/GlRequirements.cpp
        src/render/RenderThread.cpp
        src/render/UploadChannel.cpp
        src/render/ViewSlot.cpp
        src/render/ViewTargets.cpp
        src/render/device/RenderDevice.cpp
        src/render/device/material/Material.cpp
        src/render/device/texture/Texture.cpp
        HEADERS include/TechEngine/client/Client.hpp
        include/TechEngine/client/ClientError.hpp
        src/render/PacketHistory.hpp
        include/TechEngine/client/render/FrameExtraction.hpp
        include/TechEngine/client/render/FramePacket.hpp
        include/TechEngine/client/render/ImGuiHook.hpp
        include/TechEngine/client/render/RenderView.hpp
        src/render/DrawList.hpp
        src/render/FramePacketPool.hpp
        src/render/FrameView.hpp
        src/render/GpuCopies.hpp
        src/render/MaterialBuffer.hpp
        src/render/PacketMailbox.hpp
        src/render/FrameRenderer.hpp
        src/render/GlRequirements.hpp
        src/render/RenderThread.hpp
        src/render/UploadChannel.hpp
        src/render/ViewSlot.hpp
        src/render/ViewTargets.hpp
        src/render/device/RenderDevice.hpp
        src/render/device/RenderExtent.hpp
        src/render/device/RenderHandle.hpp
        src/render/device/RenderSlots.hpp
        src/render/device/buffer/Buffer.hpp
        src/render/device/draw/IndexedIndirectCommand.hpp
        src/render/device/material/Material.hpp
        src/render/device/pass/RenderPass.hpp
        src/render/device/pipeline/Pipeline.hpp
        src/render/device/pipeline/VertexLayout.hpp
        src/render/device/program/Program.hpp
        src/render/device/target/RenderTarget.hpp
        src/render/device/texture/Texture.hpp
        DEPS core platform base
        LIBS_PRIVATE TechEngine::miniaudio TechEngine::glad Threads::Threads)

if (TE_BUILD_TESTS)
    techengine_test(client SOURCES tests/ClientTests.cpp
            tests/ClientErrorTests.cpp
            tests/render/DrawListTests.cpp
            tests/render/FrameExtractionTests.cpp
            tests/render/FramePacketPoolTests.cpp
            tests/render/GeometryBuffersTests.cpp
            tests/render/GpuCopiesTests.cpp
            tests/render/MaterialBufferTests.cpp
            tests/render/PacketMailboxTests.cpp
            tests/render/PacketHistoryTests.cpp
            tests/render/FrameRendererTests.cpp
            tests/render/GlRequirementsTests.cpp
            tests/render/RenderThreadTests.cpp
            tests/render/UploadChannelTests.cpp
            tests/render/ViewSlotTests.cpp
            tests/render/ViewTargetsTests.cpp
            tests/render/device/RenderDeviceTests.cpp
            tests/render/device/RenderSlotsTests.cpp)
    target_link_libraries(TechEngineClientTests PRIVATE TechEngine::glad)
endif ()
