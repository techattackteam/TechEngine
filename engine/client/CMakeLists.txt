techengine_module(client
        SOURCES src/Client.cpp
        src/render/Buffer.cpp
        src/render/FrameCommandBuffer.cpp
        src/render/FrameRenderer.cpp
        src/render/RenderThread.cpp
        src/render/VertexArray.cpp
        HEADERS include/TechEngine/client/Client.hpp
        include/TechEngine/client/render/FrameCommand.hpp
        src/render/Buffer.hpp
        src/render/FrameCommandBuffer.hpp
        src/render/FrameRenderer.hpp
        src/render/RenderThread.hpp
        src/render/VertexArray.hpp
        DEPS core platform base
        LIBS_PRIVATE TechEngine::miniaudio TechEngine::glad Threads::Threads)

if (TE_BUILD_TESTS)
    techengine_test(client SOURCES tests/ClientTests.cpp
            tests/render/FrameCommandBufferTests.cpp
            tests/render/FrameRendererTests.cpp
            tests/render/RenderThreadTests.cpp)
    target_link_libraries(TechEngineClientTests PRIVATE TechEngine::glad)
endif ()
