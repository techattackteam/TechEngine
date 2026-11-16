techengine_module(client
        SOURCES src/Client.cpp
        src/ClientError.cpp
        src/render/SnapshotHistory.cpp
        src/render/SnapshotMailbox.cpp
        src/render/FrameRenderer.cpp
        src/render/GlRequirements.cpp
        src/render/RenderThread.cpp
        src/render/device/RenderDevice.cpp
        HEADERS include/TechEngine/client/Client.hpp
        include/TechEngine/client/ClientError.hpp
        src/render/SnapshotHistory.hpp
        include/TechEngine/client/render/ImGuiHook.hpp
        include/TechEngine/client/render/RenderSnapshot.hpp
        src/render/SnapshotMailbox.hpp
        src/render/FrameRenderer.hpp
        src/render/GlRequirements.hpp
        src/render/RenderThread.hpp
        src/render/device/RenderDevice.hpp
        src/render/device/RenderExtent.hpp
        src/render/device/RenderHandle.hpp
        src/render/device/RenderSlots.hpp
        src/render/device/buffer/Buffer.hpp
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
            tests/render/SnapshotMailboxTests.cpp
            tests/render/SnapshotHistoryTests.cpp
            tests/render/FrameRendererTests.cpp
            tests/render/GlRequirementsTests.cpp
            tests/render/RenderThreadTests.cpp
            tests/render/device/RenderDeviceTests.cpp
            tests/render/device/RenderSlotsTests.cpp)
    target_link_libraries(TechEngineClientTests PRIVATE TechEngine::glad)
endif ()
