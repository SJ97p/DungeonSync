cmake_minimum_required(VERSION 3.25)

project(DungeonSync
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(DungeonSyncClient
    Client/main.cpp
)

add_executable(DungeonSyncServer
    Server/main.cpp
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