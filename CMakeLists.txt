cmake_minimum_required(VERSION 3.25)

project(DungeonSync
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(DungeonSyncClient WIN32
    Client/main.cpp
    Client/Application.cpp
    Client/Platform/Window.cpp
    Client/Platform/Window.h
    Client/Rendering/D3D11Renderer.cpp
    Client/Rendering/D3D11Renderer.h
    Client/Rendering/WicTextureLoader.cpp
    Client/Rendering/WicTextureLoader.h
    Client/Rendering/Vertex.h
    Client/Presentation/DemoScene.cpp
    Client/Presentation/DemoScene.h
    Client/Presentation/SpriteEffectPool.cpp
    Client/Presentation/SpriteEffectPool.h
    Client/Rendering/Camera.h
    Client/Gameplay/Monster.h
    Client/Gameplay/CombatSystem.cpp
    Client/Gameplay/CombatSystem.h
    Client/Gameplay/SpatialGrid.h
    Client/Gameplay/SpatialGrid.cpp
    Client/Gameplay/DungeonRoom.h
    Client/Gameplay/DungeonController.cpp
    Client/Gameplay/DungeonController.h
    Client/Rendering/InstanceData.h
    Client/Rendering/RenderStatistics.h
    Client/Network/TcpClient.cpp
    Client/Network/TcpClient.h
    Client/Network/ServerStateReceiver.cpp
    Client/Network/ServerStateReceiver.h
    Client/Diagnostics/FrameTimeProfiler.cpp
    Client/Diagnostics/FrameTimeProfiler.h
    Client/Diagnostics/RenderingStressScene.cpp
    Client/Diagnostics/RenderingStressScene.h
    Client/Diagnostics/BenchmarkResult.h
    Client/Diagnostics/BenchmarkCsvWriter.cpp
    Client/Diagnostics/BenchmarkCsvWriter.h
    Client/Diagnostics/BenchmarkSession.cpp
    Client/Diagnostics/BenchmarkSession.h
    Shared/Network/WinsockRuntime.cpp
    Shared/Network/WinsockRuntime.h
    Shared/Network/Packet.h
)

add_executable(DungeonSyncServer
    Server/main.cpp
    Shared/Network/WinsockRuntime.cpp
    Shared/Network/WinsockRuntime.h
    Shared/Network/Packet.h
    Server/Network/TcpListener.cpp
    Server/Network/TcpListener.h
)

target_link_libraries(DungeonSyncClient PRIVATE
    d3d11
    dxgi
    d3dcompiler
    windowscodecs
    ole32
    ws2_32
)

target_link_libraries(DungeonSyncServer PRIVATE
    ws2_32
)

set(RenderingShaders
    Client/Rendering/Shaders/Triangle.hlsl
    Client/Rendering/Shaders/Ground.hlsl
)

set_source_files_properties(
    ${RenderingShaders}
    PROPERTIES HEADER_FILE_ONLY TRUE
)

target_sources(DungeonSyncClient PRIVATE
    ${RenderingShaders}
)

set_source_files_properties(
    ${TriangleShader}
    PROPERTIES HEADER_FILE_ONLY TRUE
)

target_sources(DungeonSyncClient PRIVATE
    ${TriangleShader}
)

set_target_properties(DungeonSyncClient PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

foreach(target DungeonSyncClient DungeonSyncServer)
    target_compile_definitions(${target} PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
    )

    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /utf-8
        )
    endif()
endforeach()