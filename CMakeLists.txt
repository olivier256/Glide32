cmake_minimum_required(VERSION 3.16)

project(pixel C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(MINIFB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MINIFB_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(minifb)

add_executable(pixel
    game.c
    glide.c
    vdp.c
    interface.c
    sh2.c
    platform_minifb.c
)

target_compile_options(pixel PRIVATE -Wall -Wextra)

target_link_libraries(pixel PRIVATE minifb)

set_target_properties(pixel PROPERTIES LINKER_LANGUAGE CXX)
