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
 "Client/Application.cpp" "Client/Platform/Window.cpp" "Client/Platform/Window.h" "Client/Rendering/D3D11Renderer.cpp" "Client/Rendering/D3D11Renderer.h" "Client/Rendering/Vertex.h")

add_executable(DungeonSyncServer
    Server/main.cpp
)

target_link_libraries(DungeonSyncClient PRIVATE
    d3d11
    dxgi
    d3dcompiler
)

set(TriangleShader
    Client/Rendering/Shaders/Triangle.hlsl
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