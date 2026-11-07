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

add_executable(robot_game game/main.cpp)
target_link_libraries(robot_game PRIVATE engine)

add_executable(world_tests tests/world_tests.cpp)
target_link_libraries(world_tests PRIVATE engine)

foreach(target engine robot_game world_tests)
    target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
    )
endforeach()