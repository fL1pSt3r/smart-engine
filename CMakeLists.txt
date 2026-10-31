cmake_minimum_required(VERSION 3.23)
project(smart_engine CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(smart_engine
        main.cpp
        ecs/world.cpp
)

target_compile_options(smart_engine PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
