cmake_minimum_required(VERSION 3.16)

project(pixel C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(MINIFB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MINIFB_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(minifb)

# tnl définit glBegin, glVertex3x... : si MiniFB se liait à OpenGL, nos
# fonctions prendraient silencieusement la place des vraies. Sur macOS,
# MiniFB utilise Metal (ou Cocoa seul), jamais OpenGL : on le vérifie.
if(MINIFB_USE_OPENGL_API OR USE_OPENGL_API)
    message(FATAL_ERROR "MiniFB est configuré avec OpenGL : conflit de noms avec tnl (gl*)")
endif()

add_executable(pixel
    game.c
    glide.c
    tnl.c
    sintab.c
    vdp.c
    interface.c
    sh2.c
    platform_minifb.c
)

target_compile_options(pixel PRIVATE -Wall -Wextra)

target_link_libraries(pixel PRIVATE minifb)

set_target_properties(pixel PROPERTIES LINKER_LANGUAGE CXX)

# Banc de mesure : bench.c remplace game.c
add_executable(pixel_bench
    bench.c
    glide.c
    tnl.c
    sintab.c
    vdp.c
    interface.c
    sh2.c
    platform_minifb.c
)

target_compile_options(pixel_bench PRIVATE -Wall -Wextra)

target_link_libraries(pixel_bench PRIVATE minifb)

set_target_properties(pixel_bench PROPERTIES LINKER_LANGUAGE CXX)
