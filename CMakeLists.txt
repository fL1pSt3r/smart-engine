cmake_minimum_required(VERSION 3.23)
project(smart_engine CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_library(engine STATIC
        engine/ecs/world.cpp
        engine/systems/movement.cpp
        engine/simulation/simulation.cpp
        engine/geometry/collision.cpp
)

target_include_directories(engine PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/engine"
)

add_executable(world_tests tests/world_tests.cpp)
target_link_libraries(world_tests PRIVATE engine)

foreach(target engine world_tests)
    target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
    )
endforeach()

option(SMART_ENGINE_BUILD_GAME "Build the SDL application" ON)

if(SMART_ENGINE_BUILD_GAME)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/CMakeLists.txt")
        message(FATAL_ERROR
                "SDL submodule is missing. Run: git submodule update --init --recursive")
    endif()

    add_subdirectory(external/SDL EXCLUDE_FROM_ALL)

    add_library(engine_graphics STATIC
            engine/platform/platform.cpp
            engine/renderer/renderer.cpp
    )
    target_link_libraries(engine_graphics PUBLIC engine PRIVATE SDL3::SDL3)
    target_compile_options(engine_graphics PRIVATE -Wall -Wextra -Wpedantic)

    add_executable(robot_game game/main.cpp)
    target_link_libraries(robot_game PRIVATE engine_graphics SDL3::SDL3)
    target_compile_options(robot_game PRIVATE -Wall -Wextra -Wpedantic)
endif()
